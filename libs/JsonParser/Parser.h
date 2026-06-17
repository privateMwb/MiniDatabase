#pragma once

#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Json.h"

class Parser {
private:
    // State
    std::string input_;
    std::size_t pos_    = 0;
    std::size_t line_   = 1;
    std::size_t column_ = 1;

    // Character Utilities
    [[nodiscard]] unsigned char peek() const;
    [[nodiscard]] unsigned char get();

    void skipWhitespace();
    void error(std::string_view msg) const;

    // Validation
    void ensureEndOfInput();

    // Hex and Unicode Helpers
    [[nodiscard]] int  hexDigitToInt(unsigned char c) const;
    void               appendUtf8(std::string& result, unsigned int codepoint);

    // Internal Parsers
    [[nodiscard]] Json parseValue();
    [[nodiscard]] Json parseNull();
    [[nodiscard]] Json parseBool();
    [[nodiscard]] Json parseNumber();
    [[nodiscard]] Json parseString();
    [[nodiscard]] Json parseArray();
    [[nodiscard]] Json parseObject();

public:
    // Constructor
    explicit Parser(std::string input);

    // Entry Point
    [[nodiscard]] Json parse();
};
