#include "infra/syntax/Json.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <cstdint>
#include <limits>

TEST(BasicUsageTest, object_with_some_values)
{
    infra::JsonObject object(R"({ "key" : "value", "enabled" : true, "subobject" : { "nested": true } })");

    EXPECT_EQ("value", object.GetString("key"));
    EXPECT_EQ(true, object.GetBoolean("enabled"));
    EXPECT_EQ(true, object.GetObject("subobject").GetBoolean("nested"));
}

TEST(BasicUsageTest, array_with_strings)
{
    infra::JsonArray array(R"([ "first", "second", "third" ])");

    for (auto string : JsonStringArray(array))
        EXPECT_TRUE(string == "first" || string == "second" || string == "third");
}

TEST(JsonTokenizerTest, get_end_token)
{
    infra::JsonTokenizer tokenizer(R"()");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::End()), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_empty_string_token)
{
    infra::JsonTokenizer tokenizer(R"("")");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::String("")), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_string_token)
{
    infra::JsonTokenizer tokenizer(R"("string")");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::String("string")), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_escaped_string_token)
{
    infra::JsonTokenizer tokenizer(R"("str\"ing")");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::String(R"(str\"ing)")), tokenizer.Token());
}

TEST(JsonTokenizerTest, unclosed_string_results_in_error_token)
{
    infra::JsonTokenizer tokenizer(R"("str)");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::Error()), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_end_token_after_string_token)
{
    infra::JsonTokenizer tokenizer(R"("string")");

    ASSERT_EQ(infra::JsonToken::Token(infra::JsonToken::String("string")), tokenizer.Token());
    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::End()), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_int_token)
{
    infra::JsonTokenizer tokenizer("42");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonBiggerInt(42, false)), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_negative_int_token)
{
    infra::JsonTokenizer tokenizer("-42");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonBiggerInt(42, true)), tokenizer.Token());
}

TEST(JsonTokenizerTest, skip_whitespace_before_end)
{
    infra::JsonTokenizer tokenizer(R"( )");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::End()), tokenizer.Token());
}

TEST(JsonTokenizerTest, skip_tab_whitespace_before_end)
{
    infra::JsonTokenizer tokenizer("\t");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::End()), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_true_token)
{
    infra::JsonTokenizer tokenizer(R"(true)");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::Boolean(true)), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_false_token)
{
    infra::JsonTokenizer tokenizer(R"(false)");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::Boolean(false)), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_colon_token)
{
    infra::JsonTokenizer tokenizer(R"(:)");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::Colon()), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_comma_token)
{
    infra::JsonTokenizer tokenizer(R"(,)");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::Comma()), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_dot_token)
{
    infra::JsonTokenizer tokenizer(R"(.)");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::Dot()), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_left_brace_token)
{
    infra::JsonTokenizer tokenizer(R"({)");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::LeftBrace(0)), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_right_brace_token)
{
    infra::JsonTokenizer tokenizer(R"(})");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::RightBrace(0)), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_left_bracket_token)
{
    infra::JsonTokenizer tokenizer(R"([)");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::LeftBracket(0)), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_right_bracket_token)
{
    infra::JsonTokenizer tokenizer(R"(])");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::RightBracket(0)), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_null_token)
{
    infra::JsonTokenizer tokenizer(R"(null)");
    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::Null()), tokenizer.Token());
}

TEST(JsonTokenizerTest, unknown_character_results_in_error_token)
{
    infra::JsonTokenizer tokenizer(R"(~)");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::Error()), tokenizer.Token());
}

TEST(JsonTokenizerTest, unknown_identifier_results_in_error_token)
{
    infra::JsonTokenizer tokenizer(R"(identifier)");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::Error()), tokenizer.Token());
}

TEST(JsonTokenizerTest, get_multiple_tokens)
{
    infra::JsonTokenizer tokenizer(R"({ "key" : "value" })");

    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::LeftBrace(0)), tokenizer.Token());
    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::String("key")), tokenizer.Token());
    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::Colon()), tokenizer.Token());
    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::String("value")), tokenizer.Token());
    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::RightBrace(18)), tokenizer.Token());
    EXPECT_EQ(infra::JsonToken::Token(infra::JsonToken::End()), tokenizer.Token());
}

TEST(JsonTokenizerTest, clean_json)
{
    infra::BoundedString::WithStorage<512> data(R"({ "key" : "value", "key2" : 1234, "key3" : true, "key4" : -42.1 })");
    infra::CleanJsonContents(data);
    EXPECT_EQ(R"({"key":"value","key2":1234,"key3":true,"key4":-42.1})", data);
}

TEST(JsonTokenizerTest, ValidJsonObject)
{
    EXPECT_TRUE(infra::ValidJsonObject(R"({ "key" : "value", "key2" : 1234, "key3" : true })"));
    EXPECT_FALSE(infra::ValidJsonObject(R"({ "key" })"));
}

TEST(JsonTokenizerTest, not_equal_operators)
{
    EXPECT_FALSE(infra::JsonToken::End() != infra::JsonToken::End());
    EXPECT_FALSE(infra::JsonToken::Error() != infra::JsonToken::Error());
    EXPECT_FALSE(infra::JsonToken::Colon() != infra::JsonToken::Colon());
    EXPECT_FALSE(infra::JsonToken::Comma() != infra::JsonToken::Comma());
    EXPECT_FALSE(infra::JsonToken::Dot() != infra::JsonToken::Dot());
    EXPECT_FALSE(infra::JsonToken::Null() != infra::JsonToken::Null());
    EXPECT_TRUE(infra::JsonToken::LeftBrace(0) != infra::JsonToken::LeftBrace(1));
    EXPECT_TRUE(infra::JsonToken::RightBrace(0) != infra::JsonToken::RightBrace(1));
    EXPECT_TRUE(infra::JsonToken::LeftBracket(0) != infra::JsonToken::LeftBracket(1));
    EXPECT_TRUE(infra::JsonToken::RightBracket(0) != infra::JsonToken::RightBracket(1));
    EXPECT_TRUE(infra::JsonToken::String("no") != infra::JsonToken::String("yes"));
    EXPECT_TRUE(infra::JsonToken::Boolean(true) != infra::JsonToken::Boolean(false));
}

TEST(JsonObjectIteratorTest, empty_object_iterator_compares_equal_to_end)
{
    infra::JsonObject object(R"({ })");
    infra::JsonObjectIterator iterator(object.begin());
    infra::JsonObjectIterator endIterator(object.end());

    EXPECT_EQ(endIterator, iterator);
}

TEST(JsonObjectIteratorTest, nonempty_object_iterator_does_not_compare_equal_to_end)
{
    infra::JsonObject object(R"({ "key" : "value" })");
    infra::JsonObjectIterator iterator(object.begin());
    infra::JsonObjectIterator endIterator(object.end());

    EXPECT_NE(endIterator, iterator);
}

TEST(JsonObjectIteratorTest, get_key_from_iterator)
{
    infra::JsonObject object(R"({ "key" : "value" })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ("key", iterator->key);
}

TEST(JsonObjectIteratorTest, after_next_iterator_is_end)
{
    infra::JsonObject object(R"({ "key" : "value" })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(object.end(), ++iterator);
}

TEST(JsonObjectIteratorTest, get_second_key_from_iterator)
{
    infra::JsonObject object(R"({ "key" : "value", "second_key" : "second_value" })");
    infra::JsonObjectIterator iterator(object.begin());

    ++iterator;
    EXPECT_EQ("second_key", iterator->key);
}

TEST(JsonObjectIteratorTest, get_second_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : "value", "second_key" : "second_value" })");
    infra::JsonObjectIterator iterator(object.begin());

    ++iterator;
    EXPECT_EQ("second_value", iterator->value.Get<infra::JsonString>());
}

TEST(JsonObjectIteratorTest, get_string_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : "value" })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ("value", iterator->value.Get<infra::JsonString>());
}

TEST(JsonObjectIteratorTest, get_integer_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 42 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(42, iterator->value.Get<int32_t>());
}

TEST(JsonObjectIteratorTest, get_maximum_integer_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 2147483647 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(2147483647, iterator->value.Get<int32_t>());
}

TEST(JsonObjectIteratorTest, get_minimum_integer_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : -2147483648 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(-static_cast<int64_t>(2147483648), iterator->value.Get<int32_t>());
}

TEST(JsonObjectIteratorTest, get_bigger_integer_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 123456789012 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonBiggerInt(123456789012, false), iterator->value.Get<infra::JsonBiggerInt>());
}

TEST(JsonObjectIteratorTest, get_maximum_bigger_integer_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 18446744073709551615 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonBiggerInt(18446744073709551615u, false), iterator->value.Get<infra::JsonBiggerInt>());
}

TEST(JsonObjectIteratorTest, get_minimum_bigger_integer_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : -18446744073709551615 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonBiggerInt(18446744073709551615u, true), iterator->value.Get<infra::JsonBiggerInt>());
}

TEST(JsonObjectIteratorTest, get_negative_bigger_integer_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : -123456789012 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonBiggerInt(123456789012, true), iterator->value.Get<infra::JsonBiggerInt>());
}

TEST(JsonObjectIteratorTest, get_float_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 42.1 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonFloat(42, 100000000, false), iterator->value.Get<infra::JsonFloat>());
}

TEST(JsonObjectIteratorTest, get_two_float_values_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 42.1, "key2" : 18.7 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonFloat(42, 100000000, false), iterator->value.Get<infra::JsonFloat>());
    ++iterator;
    EXPECT_EQ(infra::JsonFloat(18, 700000000, false), iterator->value.Get<infra::JsonFloat>());
}

TEST(JsonObjectIteratorTest, get_negative_float_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : -42.1 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonFloat(42, 100000000, true), iterator->value.Get<infra::JsonFloat>());
}

TEST(JsonObjectIteratorTest, get_nano_float_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 0.000000001 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonFloat(0, 1, false), iterator->value.Get<infra::JsonFloat>());
}

TEST(JsonObjectIteratorTest, get_micro_float_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 0.000001 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonFloat(0, 1000, false), iterator->value.Get<infra::JsonFloat>());
}

TEST(JsonObjectIteratorTest, get_milli_float_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 0.001 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonFloat(0, 1000000, false), iterator->value.Get<infra::JsonFloat>());
}

TEST(JsonObjectIteratorTest, get_float_value_with_more_than_nine_digits_in_fraction_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 0.1234567890123 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonFloat(0, 123456789, false), iterator->value.Get<infra::JsonFloat>());
}

TEST(JsonObjectIteratorTest, get_small_negative_float_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : -0.001 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(infra::JsonFloat(0, 1000000, true), iterator->value.Get<infra::JsonFloat>());
}

TEST(JsonObjectIteratorTest, dont_get_negative_float_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : 42.-1 })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(object.end(), iterator);
}

TEST(JsonObjectIteratorTest, get_true_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : true })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(true, iterator->value.Get<bool>());
}

TEST(JsonObjectIteratorTest, get_false_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : false })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(false, iterator->value.Get<bool>());
}

TEST(JsonObjectIteratorTest, get_object_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : { "bla" } })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(R"({ "bla" })", iterator->value.Get<infra::JsonObject>().ObjectString());
}

TEST(JsonObjectIteratorTest, get_object_value_with_nested_object_from_iterator)
{
    infra::JsonObject object(R"({ "key" : { "bla" : { } } })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(R"({ "bla" : { } })", iterator->value.Get<infra::JsonObject>().ObjectString());
}

TEST(JsonObjectIteratorTest, get_array_value_from_iterator)
{
    infra::JsonObject object(R"({ "key" : [ "bla" ] })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(R"([ "bla" ])", iterator->value.Get<infra::JsonArray>().ObjectString());
}

TEST(JsonObjectIteratorTest, get_object_value_from_iterator_with_error)
{
    infra::JsonObject object(R"({ "key" : { "bla" : ~ } })");

    EXPECT_EQ(object.end(), object.begin());
}

TEST(JsonObjectIteratorTest, get_array_value_from_iterator_with_error)
{
    infra::JsonObject object(R"({ "key" : [ "bla", ~ ] })");

    EXPECT_EQ(object.end(), object.begin());
}

TEST(JsonObjectIteratorTest, get_array_value_with_nested_array_from_iterator)
{
    infra::JsonObject object(R"({ "key" : [ "bla", [ ] ] })");
    infra::JsonObjectIterator iterator(object.begin());

    EXPECT_EQ(R"([ "bla", [ ] ])", iterator->value.Get<infra::JsonArray>().ObjectString());
}

TEST(JsonObjectTest, empty_object_construction)
{
    infra::JsonObject object(R"({ })");

    EXPECT_EQ(object.end(), object.begin());
}

TEST(JsonObjectTest, nonempty_object_construction)
{
    infra::JsonObject object(R"({ "key" : "value" })");

    EXPECT_NE(object.end(), object.begin());
}

TEST(JsonObjectTest, has_key_when_value_exists)
{
    infra::JsonObject object(R"({ "key" : "value" })");

    EXPECT_TRUE(object.HasKey("key"));
}

TEST(JsonObjectTest, has_key_when_value_does_not_exist)
{
    infra::JsonObject object(R"({ })");

    EXPECT_FALSE(object.HasKey("key"));
}

TEST(JsonObjectTest, iterate_over_object)
{
    infra::JsonObject object(R"({ "key" : "value" })");

    for (auto keyValue : object)
        EXPECT_EQ("value", keyValue.value.Get<infra::JsonString>());
}

TEST(JsonObjectTest, incorrect_object_sets_error)
{
    std::vector<infra::BoundedConstString> errorObjects = { R"(~)", R"({~})", R"({ true })", R"({ "key" ~})", R"({ "key" : ~})", R"({ "key" : [})", R"({ "key" : true ~})" };

    for (auto errorObject : errorObjects)
    {
        infra::JsonObject object(errorObject);

        for (auto keyValue : object)
        {}

        EXPECT_TRUE(object.Error());
    }
}

TEST(JsonObjectTest, get_string)
{
    infra::JsonObject object(R"({ "key" : "value" })");

    EXPECT_EQ("value", object.GetString("key"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_boolean)
{
    infra::JsonObject object(R"({ "key" : true })");

    EXPECT_EQ(true, object.GetBoolean("key"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_integer)
{
    infra::JsonObject object(R"({ "key" : 5 })");

    EXPECT_EQ(5, object.GetInteger("key"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_object)
{
    infra::JsonObject object(R"({ "key" : { "bla" } })");

    EXPECT_EQ(R"({ "bla" })", object.GetObject("key").ObjectString());
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_array)
{
    infra::JsonObject object(R"({ "key" : [ "bla" ] })");

    EXPECT_EQ(R"([ "bla" ])", object.GetArray("key").ObjectString());
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_optional_string)
{
    infra::JsonObject object(R"({ "key" : "value" })");

    EXPECT_EQ("value", *object.GetOptionalString("key"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_none_when_optional_string_is_absent)
{
    infra::JsonObject object(R"({ })");

    EXPECT_EQ(infra::none, object.GetOptionalString("key"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_optional_boolean)
{
    infra::JsonObject object(R"({ "key" : true })");

    EXPECT_EQ(true, *object.GetOptionalBoolean("key"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_none_when_optional_boolean_is_absent)
{
    infra::JsonObject object(R"({ })");

    EXPECT_EQ(infra::none, object.GetOptionalBoolean("key"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_none_when_optional_boolean_is_absent_but_key_is_present)
{
    infra::JsonObject object(R"({ "key" : "value" })");

    EXPECT_EQ(infra::none, object.GetOptionalBoolean("key"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_optional_object)
{
    infra::JsonObject object(R"({ "key" : { "bla" } })");

    EXPECT_EQ(R"({ "bla" })", object.GetOptionalObject("key")->ObjectString());
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_none_when_optional_object_is_absent)
{
    infra::JsonObject object(R"({ })");

    EXPECT_EQ(infra::none, object.GetOptionalObject("key"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_none_when_optional_array_is_absent)
{
    infra::JsonObject object(R"({ })");

    EXPECT_EQ(infra::none, object.GetOptionalArray("key"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, get_nonexistent_string_sets_error_on_object)
{
    infra::JsonObject object(R"({ })");

    EXPECT_FALSE(object.Error());
    object.GetString("key");
    EXPECT_TRUE(object.Error());
}

TEST(JsonObjectTest, nested_float_is_accepted)
{
    infra::JsonObject object(R"({ "key": { "nestedKey", 1.5 } })");

    for (auto x : object)
    {}

    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, JsonString_ToString_unescapes)
{
    infra::JsonObject object(R"({ "key": "\"\\\n\t\r\b\f\u0020\u002a\u002A" })");

    infra::BoundedString::WithStorage<32> data;
    object.GetString("key").ToString(data);
    EXPECT_EQ("\"\\\n\t\r\b\f **", data);
    EXPECT_EQ("\\\"\\\\\\n\\t\\r\\b\\f\\u0020\\u002a\\u002A", object.GetString("key").Raw());
}

TEST(JsonObjectTest, JsonString_ToString_limits_to_destination)
{
    infra::JsonObject object(R"({ "key": "\"\\\n\t\r\b\f\u0020" })");

    infra::BoundedString::WithStorage<4> data;
    object.GetString("key").ToString(data);
    EXPECT_EQ("\"\\\n\t", data);
}

TEST(JsonObjectTest, JsonString_ToStdString_unescapes)
{
    infra::JsonObject object(R"({ "key": "\"\\\n\t\r\b\f\u0020" })");

    EXPECT_EQ("\"\\\n\t\r\b\f ", object.GetString("key").ToStdString());
}

TEST(JsonObjectTest, JsonString_equality_unescapes)
{
    infra::JsonObject object(R"({ "key": "\"\\\n\t\r\b\f\u0020" })");

    EXPECT_TRUE("\"\\\n\t\r\b\f " == object.GetString("key"));
    EXPECT_TRUE(object.GetString("key") == "\"\\\n\t\r\b\f ");
    EXPECT_TRUE(infra::BoundedConstString("\"\\\n\t\r\b\f ") == object.GetString("key"));
    EXPECT_TRUE(object.GetString("key") == infra::BoundedConstString("\"\\\n\t\r\b\f "));

    EXPECT_FALSE("\"\\\n\t\r\b\f " != object.GetString("key"));
    EXPECT_FALSE(object.GetString("key") != "\"\\\n\t\r\b\f ");
    EXPECT_FALSE(infra::BoundedConstString("\"\\\n\t\r\b\f ") != object.GetString("key"));
    EXPECT_FALSE(object.GetString("key") != infra::BoundedConstString("\"\\\n\t\r\b\f "));
}

TEST(JsonObjectTest, JsonString_size)
{
    infra::JsonObject object(R"({ "key": "\"\\\n\t\r\b\f\u0020" })");

    EXPECT_EQ(8, object.GetString("key").size());
    EXPECT_FALSE(object.GetString("key").empty());
}

TEST(JsonObjectTest, JsonString_corner_case_escaping)
{
    EXPECT_EQ("\1g", infra::JsonObject(R"({ "key": "\u1g" })").GetString("key"));
    infra::JsonObject object1(R"({ "key": "\k" })");
    EXPECT_EQ("k", object1.GetString("key"));
}

TEST(JsonObjectTest, iterator_equality)
{
    infra::JsonObject object(R"({ "key1": "bla", "key2": "bla", "key3": "bla3" })");

    EXPECT_EQ(object.GetString("key1"), object.GetString("key2"));
    EXPECT_NE(object.GetString("key1"), object.GetString("key3"));
}

TEST(JsonObjectTest, null_value)
{
    infra::JsonObject object(R"({"key1":null})");

    EXPECT_TRUE(object.HasKey("key1"));
    EXPECT_FALSE(object.GetOptionalString("key1"));
    EXPECT_FALSE(object.Error());
}

TEST(JsonObjectTest, integer_conversion_uint8)
{
    EXPECT_THAT(infra::JsonObject{ R"({"key1":0})" }.GetIntegerAs<uint8_t>("key1"), testing::Eq(std::numeric_limits<uint8_t>::min()));
    EXPECT_THAT(infra::JsonObject{ R"({"key1":255})" }.GetIntegerAs<uint8_t>("key1"), testing::Eq(std::numeric_limits<uint8_t>::max()));
}

TEST(JsonObjectTest, integer_conversion_uint8_out_of_bounds)
{
    auto jsonObjectMin = infra::JsonObject{ R"({"min":-1)" };
    auto jsonObjectMax = infra::JsonObject{ R"({"max":256)" };

    EXPECT_THAT(jsonObjectMin.GetIntegerAs<uint8_t>("min"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMin.Error(), testing::IsTrue());

    EXPECT_THAT(jsonObjectMax.GetIntegerAs<uint8_t>("max"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMax.Error(), testing::IsTrue());
}

TEST(JsonObjectTest, integer_conversion_int8)
{
    EXPECT_THAT(infra::JsonObject{ R"({"key1":-128})" }.GetIntegerAs<int8_t>("key1"), testing::Eq(std::numeric_limits<int8_t>::min()));
    EXPECT_THAT(infra::JsonObject{ R"({"key1":127})" }.GetIntegerAs<int8_t>("key1"), testing::Eq(std::numeric_limits<int8_t>::max()));
}

TEST(JsonObjectTest, integer_conversion_int8_out_of_bounds)
{
    auto jsonObjectMin = infra::JsonObject{ R"({"min":-129)" };
    auto jsonObjectMax = infra::JsonObject{ R"({"max":128)" };

    EXPECT_THAT(jsonObjectMin.GetIntegerAs<int8_t>("min"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMin.Error(), testing::IsTrue());

    EXPECT_THAT(jsonObjectMax.GetIntegerAs<int8_t>("max"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMax.Error(), testing::IsTrue());
}

TEST(JsonObjectTest, integer_conversion_uint16)
{
    EXPECT_THAT(infra::JsonObject{ R"({"key1":0})" }.GetIntegerAs<uint16_t>("key1"), testing::Eq(std::numeric_limits<uint16_t>::min()));
    EXPECT_THAT(infra::JsonObject{ R"({"key1":65535})" }.GetIntegerAs<uint16_t>("key1"), testing::Eq(std::numeric_limits<uint16_t>::max()));
}

TEST(JsonObjectTest, integer_conversion_uint16_t_out_of_bounds)
{
    auto jsonObjectMin = infra::JsonObject{ R"({"min":-1)" };
    auto jsonObjectMax = infra::JsonObject{ R"({"max":65536)" };

    EXPECT_THAT(jsonObjectMin.GetIntegerAs<uint16_t>("min"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMin.Error(), testing::IsTrue());

    EXPECT_THAT(jsonObjectMax.GetIntegerAs<uint16_t>("max"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMax.Error(), testing::IsTrue());
}

TEST(JsonObjectTest, integer_conversion_int16)
{
    EXPECT_THAT(infra::JsonObject{ R"({"key1":-32768})" }.GetIntegerAs<int16_t>("key1"), testing::Eq(std::numeric_limits<int16_t>::min()));
    EXPECT_THAT(infra::JsonObject{ R"({"key1":32767})" }.GetIntegerAs<int16_t>("key1"), testing::Eq(std::numeric_limits<int16_t>::max()));
}

TEST(JsonObjectTest, integer_conversion_int16_t_out_of_bounds)
{
    auto jsonObjectMin = infra::JsonObject{ R"({"min":-32769)" };
    auto jsonObjectMax = infra::JsonObject{ R"({"max":32768)" };

    EXPECT_THAT(jsonObjectMin.GetIntegerAs<int16_t>("min"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMin.Error(), testing::IsTrue());

    EXPECT_THAT(jsonObjectMax.GetIntegerAs<int16_t>("max"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMax.Error(), testing::IsTrue());
}

TEST(JsonObjectTest, integer_conversion_uint32)
{
    EXPECT_THAT(infra::JsonObject{ R"({"key1":0})" }.GetIntegerAs<uint32_t>("key1"), testing::Eq(std::numeric_limits<uint32_t>::min()));
    EXPECT_THAT(infra::JsonObject{ R"({"key1":4294967295})" }.GetIntegerAs<uint32_t>("key1"), testing::Eq(std::numeric_limits<uint32_t>::max()));
}

TEST(JsonObjectTest, integer_conversion_uint32_t_out_of_bounds)
{
    auto jsonObjectMin = infra::JsonObject{ R"({"min":-1)" };
    auto jsonObjectMax = infra::JsonObject{ R"({"max":4294967296)" };

    EXPECT_THAT(jsonObjectMin.GetIntegerAs<uint32_t>("min"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMin.Error(), testing::IsTrue());

    EXPECT_THAT(jsonObjectMax.GetIntegerAs<uint32_t>("max"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMax.Error(), testing::IsTrue());
}

TEST(JsonObjectTest, integer_conversion_int32)
{
    EXPECT_THAT(infra::JsonObject{ R"({"key1":-2147483648})" }.GetIntegerAs<int32_t>("key1"), testing::Eq(std::numeric_limits<int32_t>::min()));
    EXPECT_THAT(infra::JsonObject{ R"({"key1":2147483647})" }.GetIntegerAs<int32_t>("key1"), testing::Eq(std::numeric_limits<int32_t>::max()));
}

TEST(JsonObjectTest, integer_conversion_int32_t_out_of_bounds)
{
    auto jsonObjectMin = infra::JsonObject{ R"({"min":-2147483649)" };
    auto jsonObjectMax = infra::JsonObject{ R"({"max":2147483648)" };

    EXPECT_THAT(jsonObjectMin.GetIntegerAs<int32_t>("min"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMin.Error(), testing::IsTrue());

    EXPECT_THAT(jsonObjectMax.GetIntegerAs<int32_t>("max"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMax.Error(), testing::IsTrue());
}

TEST(JsonObjectTest, integer_conversion_uint64)
{
    EXPECT_THAT(infra::JsonObject{ R"({"key1":0})" }.GetIntegerAs<uint64_t>("key1"), testing::Eq(std::numeric_limits<uint64_t>::min()));
    EXPECT_THAT(infra::JsonObject{ R"({"key1":18446744073709551615})" }.GetIntegerAs<uint64_t>("key1"), testing::Eq(std::numeric_limits<uint64_t>::max()));
}

TEST(JsonObjectTest, integer_conversion_uint64_t_out_of_bounds)
{
    auto jsonObjectMin = infra::JsonObject{ R"({"min":-1)" };
    // testing for a overflow of uint64_t max is not possible

    EXPECT_THAT(jsonObjectMin.GetIntegerAs<uint64_t>("min"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMin.Error(), testing::IsTrue());
}

TEST(JsonObjectTest, integer_conversion_int64)
{
    EXPECT_THAT(infra::JsonObject{ R"({"key1":-9223372036854775808})" }.GetIntegerAs<int64_t>("key1"), testing::Eq(std::numeric_limits<int64_t>::min()));
    EXPECT_THAT(infra::JsonObject{ R"({"key1":9223372036854775807})" }.GetIntegerAs<int64_t>("key1"), testing::Eq(std::numeric_limits<int64_t>::max()));
}

TEST(JsonObjectTest, integer_conversion_int64_t_out_of_bounds)
{
    auto jsonObjectMin = infra::JsonObject{ R"({"min":-9223372036854775809)" };
    auto jsonObjectMax = infra::JsonObject{ R"({"max":9223372036854775808)" };

    EXPECT_THAT(jsonObjectMin.GetIntegerAs<int64_t>("min"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMin.Error(), testing::IsTrue());

    EXPECT_THAT(jsonObjectMax.GetIntegerAs<int64_t>("max"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMax.Error(), testing::IsTrue());
}

TEST(JsonObjectTest, integer_conversion_not_a_number)
{
    auto jsonObject = infra::JsonObject{ R"({"key":"not-a-number")" };

    EXPECT_THAT(jsonObject.GetIntegerAs<int64_t>("key"), testing::Eq(0));
    EXPECT_THAT(jsonObject.Error(), testing::IsTrue());
}

TEST(JsonObjectTest, integer_conversion_unsigned_overflow)
{
    auto jsonObject = infra::JsonObject{ R"({"key":18446744073709551615)" };

    EXPECT_THAT(jsonObject.GetIntegerAs<int64_t>("key"), testing::Eq(0));
    EXPECT_THAT(jsonObject.Error(), testing::IsTrue());
}

TEST(JsonObjectTest, integer_conversion_signed_overflow)
{
    auto jsonObjectMin = infra::JsonObject{ R"({"key":-9223372036854775808)" };
    auto jsonObjectMax = infra::JsonObject{ R"({"key":9223372036854775807)" };

    EXPECT_THAT(jsonObjectMin.GetIntegerAs<int8_t>("key"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMin.Error(), testing::IsTrue());

    EXPECT_THAT(jsonObjectMax.GetIntegerAs<int8_t>("key"), testing::Eq(0));
    EXPECT_THAT(jsonObjectMax.Error(), testing::IsTrue());
}

TEST(JsonArrayIteratorTest, empty_array_iterator_compares_equal_to_end)
{
    infra::JsonArray jsonArray(R"([ ])");
    infra::JsonArrayIterator iterator(jsonArray.begin());
    infra::JsonArrayIterator endIterator(jsonArray.end());

    EXPECT_EQ(endIterator, iterator);
}

TEST(JsonArrayIteratorTest, nonempty_array_iterator_does_not_compare_equal_to_end)
{
    infra::JsonArray jsonArray(R"([ "key" : "value" ])");
    infra::JsonArrayIterator iterator(jsonArray.begin());
    infra::JsonArrayIterator endIterator(jsonArray.end());

    EXPECT_NE(endIterator, iterator);
}

TEST(JsonObjectIteratorTest, get_value_from_iterator)
{
    infra::JsonArray jsonArray(R"([ "value" ])");
    infra::JsonArrayIterator iterator(jsonArray.begin());

    EXPECT_EQ("value", iterator->Get<infra::JsonString>());
}

TEST(JsonObjectIteratorTest, get_multiple_values_from_iterator)
{
    infra::JsonArray jsonArray(R"([ "value", true, { "subobject" } ])");
    infra::JsonArrayIterator iterator(jsonArray.begin());

    EXPECT_EQ("value", (iterator++)->Get<infra::JsonString>());
    EXPECT_EQ(true, (iterator++)->Get<bool>());
    EXPECT_EQ(R"({ "subobject" })", (*iterator++).Get<infra::JsonObject>().ObjectString());
}

TEST(JsonObjectIteratorTest, only_string_open)
{
    infra::JsonObject object(R"({")");
    for (auto j : object)
    {}

    EXPECT_TRUE(object.Error());
}

TEST(JsonArrayTest, empty_array_construction)
{
    infra::JsonArray jsonArray(R"([ ])");

    EXPECT_EQ(jsonArray.end(), jsonArray.begin());
}

TEST(JsonArrayTest, nonempty_array_construction)
{
    infra::JsonArray jsonArray(R"([ "value" ])");

    EXPECT_NE(jsonArray.end(), jsonArray.begin());
}

TEST(JsonArrayTest, iterate_over_array)
{
    infra::JsonArray jsonArray(R"([ "value" ])");

    for (auto value : jsonArray)
        EXPECT_EQ("value", value.Get<infra::JsonString>());

    EXPECT_EQ(1, std::distance(jsonArray.begin(), jsonArray.end()));
}

TEST(JsonArrayTest, iterate_over_strings_in_array)
{
    infra::JsonArray jsonArray(R"([ "value" ])");

    for (auto string : JsonStringArray(jsonArray))
        EXPECT_EQ("value", string);

    EXPECT_EQ(1, std::distance(JsonStringArray(jsonArray).first, JsonStringArray(jsonArray).second));
}

TEST(JsonArrayTest, iterate_over_strings_in_array_with_other_values_sets_error)
{
    infra::JsonArray jsonArray(R"([ true, "value" ])");

    for (auto string : JsonStringArray(jsonArray))
    {}

    EXPECT_TRUE(jsonArray.Error());
}

TEST(JsonArrayTest, iterate_over_booleans_in_array)
{
    infra::JsonArray jsonArray(R"([ true ])");

    for (auto boolean : JsonBooleanArray(jsonArray))
        EXPECT_EQ(true, boolean);

    EXPECT_EQ(1, std::distance(JsonBooleanArray(jsonArray).first, JsonBooleanArray(jsonArray).second));
}

TEST(JsonArrayTest, iterate_over_integers_in_array)
{
    infra::JsonArray jsonArray(R"([ 5 ])");

    for (auto integer : JsonIntegerArray(jsonArray))
        EXPECT_EQ(5, integer);

    EXPECT_EQ(1, std::distance(JsonIntegerArray(jsonArray).first, JsonIntegerArray(jsonArray).second));
}

TEST(JsonArrayTest, iterate_over_objects_in_array)
{
    infra::JsonArray jsonArray(R"([ { "name": "Richard" } ])");

    for (auto object : JsonObjectArray(jsonArray))
        EXPECT_EQ(R"({ "name": "Richard" })", object.ObjectString());

    EXPECT_EQ(1, std::distance(JsonObjectArray(jsonArray).first, JsonObjectArray(jsonArray).second));
}

TEST(JsonArrayTest, iterate_over_arrays_in_array)
{
    infra::JsonArray jsonArray(R"([ [ true ] ])");

    for (auto array : JsonArrayArray(jsonArray))
        EXPECT_EQ("[ true ]", array.ObjectString());

    EXPECT_EQ(1, std::distance(JsonArrayArray(jsonArray).first, JsonArrayArray(jsonArray).second));
}

TEST(JsonArrayTest, incorrect_array_sets_error)
{
    std::vector<infra::BoundedConstString> errorArrays = { R"(~)", R"([~])", R"([ "value" ~])", R"([ "value" , ~])", R"([ { ])" };

    for (auto errorArray : errorArrays)
    {
        infra::JsonArray jsonArray(errorArray);

        for (auto value : jsonArray)
        {}

        EXPECT_TRUE(jsonArray.Error());
    }
}

TEST(JsonArrayTest, nested_float_is_accepted)
{
    infra::JsonArray jsonArray(R"([ { 1.5 } ])");

    for (auto x : jsonArray)
    {}

    EXPECT_FALSE(jsonArray.Error());
}
