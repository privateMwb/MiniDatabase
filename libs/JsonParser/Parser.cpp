#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Parser.h"

// Constructor
Parser::Parser(std::string input) :
    input_(std::move(input)),
    pos_(0) {}

// Character Utilities
unsigned char Parser::peek() const {
    if (pos_ >= input_.size())
        return '\0';

    return static_cast<unsigned char>(input_[pos_]);
}

unsigned char Parser::get() {
    if (pos_ >= input_.size())
        return '\0';

    unsigned char c = static_cast<unsigned char>(input_[pos_++]);

    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }

    return c;
}

void Parser::skipWhitespace() {
    while (std::isspace(peek()))
        (void)get();
}

void Parser::error(std::string_view msg) const {
    throw std::runtime_error(
        "line "  + std::to_string(line_)   +
        ", col " + std::to_string(column_) +
        ": "     + std::string(msg));
}

// Validation
void Parser::ensureEndOfInput() {
    skipWhitespace();

    if (peek() != '\0')
        error("unexpected trailing characters");
}

// Hex and Unicode Helpers
int Parser::hexDigitToInt(unsigned char c) const {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;

    error("invalid hexadecimal digit");

    return 0; // unreachable — error() always throws
}

void Parser::appendUtf8(std::string& result, unsigned int codepoint) {
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
        error("invalid surrogate codepoint");

    if (codepoint <= 0x7F) {
        result += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        result += static_cast<char>(0xC0 | (codepoint >> 6));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        result += static_cast<char>(0xE0 | (codepoint >> 12));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0x10FFFF) {
        result += static_cast<char>(0xF0 | (codepoint >> 18));
        result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        error("invalid Unicode codepoint");
    }
}

// Internal Parsers
Json Parser::parseValue() {
    skipWhitespace();

    switch (peek()) {
        case 'n':
            return parseNull();

        case 't':
        case 'f':
            return parseBool();

        case '"':
            return parseString();

        case '[':
            return parseArray();

        case '{':
            return parseObject();

        default:
            if (std::isdigit(peek()) || peek() == '-')
                return parseNumber();

            error("invalid JSON value");
    }

    return Json(); // unreachable — error() always throws
}

Json Parser::parseNull() {
    std::string_view view(input_);

    if (view.substr(pos_, 4) != "null")
        error("expected 'null'");

    pos_ += 4;

    return Json(nullptr);
}

Json Parser::parseBool() {
    std::string_view view(input_);

    if (view.substr(pos_, 4) == "true") {
        pos_ += 4;
        return Json(true);
    }

    if (view.substr(pos_, 5) == "false") {
        pos_ += 5;
        return Json(false);
    }

    error("expected 'true' or 'false'");

    return Json(); // unreachable — error() always throws
}

Json Parser::parseNumber() {
    std::size_t start = pos_;

    if (peek() == '-')
        (void)get();

    if (!std::isdigit(peek()))
        error("expected digit");

    if (peek() == '0') {
        (void)get();

        if (std::isdigit(peek()))
            error("leading zeros are not allowed");
    } else {
        while (std::isdigit(peek()))
            (void)get();
    }

    if (peek() == '.') {
        (void)get();

        if (!std::isdigit(peek()))
            error("expected digit after decimal point");

        while (std::isdigit(peek()))
            (void)get();
    }

    if (peek() == 'e' || peek() == 'E') {
        (void)get();

        if (peek() == '+' || peek() == '-')
            (void)get();

        if (!std::isdigit(peek()))
            error("expected exponent digit");

        while (std::isdigit(peek()))
            (void)get();
    }

    return Json(std::stod(input_.substr(start, pos_ - start)));
}

Json Parser::parseString() {
    if (get() != '"')
        error("expected '\"'");

    std::string result;

    while (true) {
        unsigned char c = get();

        if (c == '\0')
            error("unterminated string");

        if (c == '"')
            break;

        if (c == '\\') {
            unsigned char esc = get();

            if (esc == '\0')
                error("unterminated escape sequence");

            switch (esc) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'n':  result += '\n'; break;
                case 't':  result += '\t'; break;
                case 'r':  result += '\r'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;

                case 'u': {
                    unsigned int codepoint = 0;

                    for (int i = 0; i < 4; ++i) {
                        codepoint <<= 4;
                        codepoint |= hexDigitToInt(get());
                    }

                    // handle surrogate pairs
                    if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                        if (get() != '\\') error("invalid surrogate pair");
                        if (get() != 'u')  error("invalid surrogate pair");

                        unsigned int low = 0;

                        for (int i = 0; i < 4; ++i) {
                            low <<= 4;
                            low |= hexDigitToInt(get());
                        }

                        if (low < 0xDC00 || low > 0xDFFF)
                            error("invalid low surrogate");

                        codepoint = 0x10000
                                  + ((codepoint - 0xD800) << 10)
                                  + (low - 0xDC00);
                    }

                    appendUtf8(result, codepoint);
                    break;
                }

                default:
                    error("invalid escape sequence");
            }
        } else {
            result += static_cast<char>(c);
        }
    }

    return Json(std::move(result));
}

Json Parser::parseArray() {
    if (get() != '[')
        error("expected '['");

    Json::ArrayType arr;

    skipWhitespace();

    if (peek() == ']') {
        (void)get();
        return Json(std::move(arr));
    }

    while (true) {
        arr.push_back(parseValue());

        skipWhitespace();

        if (peek() == ',') {
            (void)get();
            skipWhitespace();
        } else if (peek() == ']') {
            (void)get();
            break;
        } else {
            error("expected ',' or ']'");
        }
    }

    return Json(std::move(arr));
}

Json Parser::parseObject() {
    if (get() != '{')
        error("expected '{'");

    Json::ObjectType obj;

    skipWhitespace();

    if (peek() == '}') {
        (void)get();
        return Json(std::move(obj));
    }

    while (true) {
        skipWhitespace();

        if (peek() != '"')
            error("expected string key");

        std::string key = parseString().asString();

        skipWhitespace();

        if (get() != ':')
            error("expected ':'");

        obj[std::move(key)] = parseValue();

        skipWhitespace();

        if (peek() == ',') {
            (void)get();
        } else if (peek() == '}') {
            (void)get();
            break;
        } else {
            error("expected ',' or '}'");
        }
    }

    return Json(std::move(obj));
}

// Entry Point
Json Parser::parse() {
    skipWhitespace();

    Json value = parseValue();

    ensureEndOfInput();

    return value;
}
