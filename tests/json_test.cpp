#include "json.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>
#include <cstdio>

TEST_CASE("strict JSON replaces non-finite numeric tokens", "[json]") {
	const std::vector<std::pair<std::string, std::string>> cases {
	    {R"({"value":NaN})", R"({"value":null})"},
	    {R"({"value":Infinity})", R"({"value":null})"},
	    {R"({"value":-Infinity})", R"({"value":null})"},
	    {R"({"values":[NaN,Infinity,-Infinity,1.5]})", R"({"values":[null,null,null,1.5]})"},
	    {R"({"nested":{"value":NaN}})", R"({"nested":{"value":null}})"},
	    {R"({"text":"NaN Infinity -Infinity"})", R"({"text":"NaN Infinity -Infinity"})"},
	    {R"({"text":"escaped quote: \"NaN\""})", R"({"text":"escaped quote: \"NaN\""})"},
	    {R"({"word":"NotNaN","value":1})", R"({"word":"NotNaN","value":1})"},
	    {R"([NaN])", R"([null])"},
	    {"NaN", "null"},
	    {"Infinity", "null"},
	    {"-Infinity", "null"},
	};

	for (std::size_t index = 0; index < cases.size(); ++index) {
		const auto actual = sqrail::StrictJson(cases[index].first);
		CAPTURE(index, cases[index].first);
		CHECK(actual == cases[index].second);
		CHECK(sqrail::StrictJson(actual) == actual);
	}
}

TEST_CASE("JSON escaping emits portable Unicode and replacements", "[json]") {
	CHECK(sqrail::JsonEscape("a\n\"b\\") == R"(a\n\"b\\)");
	CHECK(sqrail::JsonEscape("\xE6\x97\xA5\xF0\x9F\x98\x80") == R"(\u65e5\ud83d\ude00)");
	CHECK(sqrail::JsonEscape(std::string(1, static_cast<char>(0xFF))) == R"(\ufffd)");
	CHECK(sqrail::JsonEscape("\xC0\xAF") == R"(\ufffd\ufffd)");
}

TEST_CASE("JSON escaping covers all ASCII control bytes 0x00-0x1F", "[json][escape]") {
    CHECK(sqrail::JsonEscape(std::string(1, '\b')) == "\\b");
    CHECK(sqrail::JsonEscape(std::string(1, '\t')) == "\\t");
    CHECK(sqrail::JsonEscape(std::string(1, '\n')) == "\\n");
    CHECK(sqrail::JsonEscape(std::string(1, '\f')) == "\\f");
    CHECK(sqrail::JsonEscape(std::string(1, '\r')) == "\\r");
    for (unsigned b = 0; b <= 0x1Fu; ++b) {
        if (b == 0x08u || b == 0x09u || b == 0x0Au || b == 0x0Cu || b == 0x0Du) continue;
        const std::string in(1, static_cast<char>(b));
        const std::string out = sqrail::JsonEscape(in);
        char expected[8];
        std::snprintf(expected, sizeof(expected), "\\u%04x", b);
        CHECK(out == expected);
    }
}

TEST_CASE("JSON escaping replaces truncated UTF-8 sequences with U+FFFD", "[json][escape][utf8]") {
    using std::string;
    CHECK(sqrail::JsonEscape(string(1, static_cast<char>(0x80))) == "\\ufffd");
    CHECK(sqrail::JsonEscape(string(1, static_cast<char>(0xC2))) == "\\ufffd");
    CHECK(sqrail::JsonEscape(string{static_cast<char>(0xE0), static_cast<char>(0xA0)}) == "\\ufffd");
    CHECK(sqrail::JsonEscape(string{static_cast<char>(0xF0), static_cast<char>(0x9F), static_cast<char>(0x98)}) == "\\ufffd");
    const auto out = sqrail::JsonEscape(string{static_cast<char>(0xC0), static_cast<char>(0x80)});
    CHECK(out.find("\\ufffd") != std::string::npos);
}
