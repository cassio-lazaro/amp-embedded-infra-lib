#include "infra/stream/CountingInputStream.hpp"
#include "infra/stream/LimitedInputStream.hpp"
#include "infra/stream/LimitedOutputStream.hpp"
#include "infra/stream/SavedMarkerStream.hpp"
#include "infra/stream/StringInputStream.hpp"
#include "infra/stream/StringOutputStream.hpp"
#include "services/network/HttpErrors.hpp"
#include "services/network/HttpServer.hpp"
#include <limits>

namespace services
{
    void HttpPageServer::AddPage(services::HttpPage& page)
    {
        pages.push_front(page);
    }

    services::HttpPage* HttpPageServer::PageForRequest(const HttpRequestParser& request)
    {
        for (auto& page : pages)
            if (page.ServesRequest(request.PathTokens()))
                return &page;

        return nullptr;
    }

    HttpResponseHeaderBuilder::HttpResponseHeaderBuilder(infra::TextOutputStream& output)
        : output(output)
    {
        output << "HTTP/1.1 ";
    }

    HttpResponseHeaderBuilder::HttpResponseHeaderBuilder(infra::TextOutputStream& output, infra::BoundedConstString status)
        : output(output)
    {
        output << "HTTP/1.1 " << status;
    }

    void HttpResponseHeaderBuilder::AddHeader(infra::BoundedConstString key, infra::BoundedConstString value)
    {
        output << "\r\n" << key << ": " << value;
    }

    void HttpResponseHeaderBuilder::AddHeader(infra::BoundedConstString key, uint32_t value)
    {
        output << "\r\n" << key << ": " << value;
    }

    void HttpResponseHeaderBuilder::AddHeader(infra::BoundedConstString key)
    {
        output << "\r\n" << key << ": ";
    }

    void HttpResponseHeaderBuilder::StartBody()
    {
        output << "\r\n\r\n";
    }

    infra::TextOutputStream& HttpResponseHeaderBuilder::Stream()
    {
        return output;
    }

    HttpResponse::HttpResponse(std::size_t maxBodySize)
        : maxBodySize(maxBodySize)
    {}

    void HttpResponse::WriteResponse(infra::TextOutputStream& stream) const
    {
        HttpResponseHeaderBuilder builder(stream);
        auto contentType = ContentType();
        std::size_t resultMarker = stream.SaveMarker();
        if (!contentType.empty())
            builder.AddHeader("Content-Length");
        std::size_t sizeMarker = stream.SaveMarker();
        if (!contentType.empty())
            builder.AddHeader("Content-Type", contentType);
        AddHeaders(builder);

        builder.StartBody();
        uint32_t sizeBeforeGetResponse = stream.ProcessedBytesSince(sizeMarker);
        infra::LimitedTextOutputStream limitedResponse(stream.Writer(), maxBodySize);
        WriteBody(limitedResponse);

        if (!contentType.empty())
        {
            uint32_t size = stream.ProcessedBytesSince(sizeMarker) - sizeBeforeGetResponse;
            infra::SavedMarkerTextStream sizeStream(stream, sizeMarker);
            sizeStream << size;
        }

        {
            infra::SavedMarkerTextStream markerStream(stream, resultMarker);
            markerStream << Status();
        }
    }

    infra::BoundedConstString HttpResponse::ContentType() const
    {
        return infra::BoundedConstString();
    }

    void HttpResponse::AddHeaders(HttpResponseHeaderBuilder& builder) const
    {}

    HttpServerConnectionObserver::HttpServerConnectionObserver(infra::BoundedString& buffer, HttpPageServer& httpServer)
        : buffer(buffer)
        , httpServer(httpServer)
        , initialIdle(std::chrono::seconds(10), [this]()
            {
                idle = true;
                CheckIdleClose();
            })
    {}

    void HttpServerConnectionObserver::Attached()
    {
        this->connection = &Subject();
        RequestSendStream();
    }

    void HttpServerConnectionObserver::SendStreamAvailable(infra::SharedPtr<infra::StreamWriter>&& writer)
    {
        streamWriter = std::move(writer);

        if (sendingResponse)
        {
            SendBuffer();
            PrepareForNextRequest();
        }
        else
            DataReceived();
    }

    void HttpServerConnectionObserver::DataReceived()
    {
        if (streamWriter == nullptr)
            return;

        infra::SharedPtr<infra::StreamReaderWithRewinding> reader = Subject().ReceiveStream();
        readerPtr = &reader;

        infra::WeakPtr<ConnectionObserver> weakSelf = Subject().ObserverPtr();

        if (!reader->Empty())
            if (pageServer != nullptr)
                DataReceivedForPage(*reader);
            else
                ReceivedRequest(*reader);

        if (weakSelf.lock())
        {
            if (IsAttached())
                Subject().AckReceived();
            readerPtr = nullptr;
        }
    }

    void HttpServerConnectionObserver::Detaching()
    {
        if (pageServer != nullptr && contentLength == infra::none)
        {
            parser->SetContentLength(lengthRead);
            contentLength = 0;
            infra::StringInputStreamReader reader("");
            DataReceivedForPage(reader);
        }

        streamWriter = nullptr;

        if (readerPtr != nullptr)
            *readerPtr = nullptr;
    }

    void HttpServerConnectionObserver::Close()
    {
        closeWhenIdle = true;
        CheckIdleClose();
    }

    void HttpServerConnectionObserver::Abort()
    {
        // TakeOverConnection may have been invoked, which leads to Subject() not being available. However, TakeOverConnection may have been invoked by a page
        // which is about to upgrade to a web connection, but which is not yet upgraded since it is waiting for storage. In that case, if the HttpServer
        // must close down, then we still need to invoke CloseAndDestroy on the connection. Therefore, connection is used instead of Subject()
        connection->AbortAndDestroy();
    }

    void HttpServerConnectionObserver::SendResponse(const HttpResponse& response)
    {
        SendResponseWithoutNextRequest(response);
        PrepareForNextRequest();
    }

    void HttpServerConnectionObserver::SendResponseWithoutNextRequest(const HttpResponse& response)
    {
        buffer.clear();
        infra::StringOutputStream responseStream(buffer);
        response.WriteResponse(responseStream);
        SendingHttpResponse(buffer);

        infra::TextOutputStream::WithErrorPolicy stream(*streamWriter);

        if (send100Response)
            stream << "HTTP/1.1 100 Continue\r\nContent-Length: 0\r\nContent-Type: application/json\r\nStrict-Transport-Security: max-age=31536000\r\n\r\n";

        SendBuffer();
    }

    void HttpServerConnectionObserver::TakeOverConnection(ConnectionObserver& newObserver)
    {
        streamWriter = nullptr;
        Subject().AckReceived();

        auto& connection = Subject();
        auto newObserverPtr = infra::MakeContainedSharedObject(newObserver, connection.ObserverPtr());
        Detach();
        connection.Attach(newObserverPtr);
    }

    void HttpServerConnectionObserver::SetIdle()
    {
        idle = true;
        CheckIdleClose();
    }

    HttpPage* HttpServerConnectionObserver::PageForRequest(const HttpRequestParser& request)
    {
        auto page = HttpPageServer::PageForRequest(request);
        if (page != nullptr)
            return page;
        else
            return httpServer.PageForRequest(request);
    }

    void HttpServerConnectionObserver::ReceivedTooMuchData(infra::StreamReader& reader)
    {
        infra::TextInputStream::WithErrorPolicy stream(reader);
        while (!stream.Empty())
            stream.ContiguousRange();
        Subject().AckReceived();
        SendResponse(HttpResponseOutOfMemory::Instance());
    }

    void HttpServerConnectionObserver::ReceivedRequest(infra::StreamReaderWithRewinding& reader)
    {
        infra::TextInputStream::WithErrorPolicy stream(reader);
        auto start = reader.ConstructSaveMarker();
        auto available = std::min(stream.Available(), buffer.max_size() - buffer.size());
        buffer.resize(buffer.size() + available);
        auto justReceived = buffer.substr(buffer.size() - available);
        stream >> justReceived;

        // First eat up any leftover of previous requests
        auto reducedContentsLength = std::min<uint32_t>(contentLength.ValueOr(0), buffer.size());
        buffer.erase(buffer.begin(), buffer.begin() + reducedContentsLength);
        if (contentLength != infra::none)
            *contentLength -= reducedContentsLength;

        parser.Emplace(buffer);
        if (parser->HeadersComplete())
        {
            reader.Rewind(start + buffer.size());
            Subject().AckReceived();
            TryHandleRequest(reader);
        }
        else if (!reader.Empty())
            ReceivedTooMuchData(reader);
        else
            Subject().AckReceived();
    }

    void HttpServerConnectionObserver::TryHandleRequest(infra::StreamReaderWithRewinding& reader)
    {
        if (!parser->Valid())
            SendResponse(HttpResponseBadRequest::Instance());
        else
            HandleRequest(reader);
    }

    void HttpServerConnectionObserver::HandleRequest(infra::StreamReaderWithRewinding& reader)
    {
        RequestIsNowInProgress();

        send100Response |= Expect100();

        ServePage(reader);
    }

    void HttpServerConnectionObserver::RequestIsNowInProgress()
    {
        idle = false;
        initialIdle.Cancel();
        ReceivedHttpRequest(buffer);
    }

    void HttpServerConnectionObserver::ServePage(infra::StreamReaderWithRewinding& reader)
    {
        contentLength = parser->ContentLength();
        pageServer = PageForRequest(*parser);
        if (pageServer != nullptr)
        {
            pageServer->RequestReceived(*parser, *this);
            if (pageServer != nullptr)
                DataReceivedForPage(reader);
        }
        else
            SendResponse(HttpResponseNotFound::Instance());
    }

    void HttpServerConnectionObserver::DataReceivedForPage(infra::StreamReaderWithRewinding& reader)
    {
        if (contentLength != infra::none && reader.Available() > *contentLength)
            Abort();
        else
        {
            infra::LimitedStreamReaderWithRewinding::WithInput<infra::CountingStreamReaderWithRewinding> countingReader(infra::inPlace, reader, contentLength.ValueOr(std::numeric_limits<uint32_t>::max()));
            pageServer->DataReceived(countingReader);
            if (contentLength != infra::none)
                *contentLength -= countingReader.Storage().TotalRead();
            lengthRead += countingReader.Storage().TotalRead();
        }
    }

    void HttpServerConnectionObserver::RequestSendStream()
    {
        streamWriter = nullptr;
        Subject().RequestSendStream(Subject().MaxSendStreamSize());
    }

    void HttpServerConnectionObserver::PrepareForNextRequest()
    {
        RequestSendStream();
        if (!sendingResponse)
        {
            pageServer = nullptr;
            parser = infra::none;
            lengthRead = 0;
            send100Response = false;
            buffer.clear();
            SetIdle();
        }
    }

    bool HttpServerConnectionObserver::Expect100() const
    {
        return parser->Header("Expect") == "100-continue";
    }

    void HttpServerConnectionObserver::SendBuffer()
    {
        infra::TextOutputStream::WithErrorPolicy stream(*streamWriter);

        auto available = stream.Available();
        stream << buffer.substr(0, available);
        buffer.erase(0, available);
        sendingResponse = !buffer.empty();
    }
    
    void HttpServerConnectionObserver::CheckIdleClose()
    {
        if (closeWhenIdle && idle)
            ConnectionObserver::Close();
    }

    void SimpleHttpPage::RequestReceived(HttpRequestParser& parser, HttpServerConnection& connection)
    {
        this->connection = &connection;
        this->parser = &parser;
    }

    void SimpleHttpPage::DataReceived(infra::StreamReaderWithRewinding& reader)
    {
        infra::TextInputStream::WithErrorPolicy stream(reader);
        auto& buffer = parser->BodyBuffer();
        auto available = stream.Available();
        if (available > buffer.max_size() - buffer.size() || buffer.size() + available > parser->ContentLength().ValueOr(std::numeric_limits<uint32_t>::max()))
        {
            while (!stream.Empty())
                stream.ContiguousRange();
            connection->SendResponse(HttpResponseOutOfMemory::Instance());
        }
        else
        {
            auto available = stream.Available();
            buffer.resize(buffer.size() + available);
            auto justReceived = buffer.substr(buffer.size() - available);
            stream >> justReceived;

            if (buffer.size() == parser->ContentLength())
                RespondToRequest(*parser, *connection);
        }
    }

    DefaultHttpServer::DefaultHttpServer(infra::BoundedString& buffer, ConnectionFactory& connectionFactory, uint16_t port)
        : SingleConnectionListener(connectionFactory, port, { connectionCreator })
        , buffer(buffer)
        , connectionCreator([this](infra::Optional<HttpServerConnectionObserver>& value, IPAddress address)
            {
                value.Emplace(this->buffer, *this);
            })
    {}
}
