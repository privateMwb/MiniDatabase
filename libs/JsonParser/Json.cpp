#include <sstream>
#include <stdexcept>
#include <string>
#include <ostream>

#include "Json.h"
#include "Parser.h"

namespace {
    // returns a string of n spaces for indentation
    std::string pad(std::size_t n) {
        return std::string(n, ' ');
    }
}

// Constructors & Destructor
Json::Json() :
    type_(Type::Null),
    value_(nullptr) {}

Json::Json(std::nullptr_t) :
    type_(Type::Null),
    value_(nullptr) {}

Json::Json(bool value) :
    type_(Type::Bool),
    value_(value) {}

Json::Json(double value) :
    type_(Type::Number),
    value_(value) {}

Json::Json(int value) :
    type_(Type::Number),
    value_(static_cast<double>(value)) {}

Json::Json(const std::string& value) :
    type_(Type::String),
    value_(value) {}

Json::Json(std::string&& value) :
    type_(Type::String),
    value_(std::move(value)) {}

Json::Json(const char* value) :
    type_(Type::String),
    value_(std::string(value)) {}

Json::Json(const ArrayType& value) :
    type_(Type::Array),
    value_(value) {}

Json::Json(ArrayType&& value) :
    type_(Type::Array),
    value_(std::move(value)) {}

Json::Json(const ObjectType& value) :
    type_(Type::Object),
    value_(value) {}

Json::Json(ObjectType&& value) :
    type_(Type::Object),
    value_(std::move(value)) {}

Json::Json(const Json& other) :
    type_(other.type_),
    value_(other.value_) {}

Json& Json::operator=(const Json& other) {
    if (this != &other) {
        type_  = other.type_;
        value_ = other.value_;
    }

    return *this;
}

Json::Json(Json&& other) noexcept :
    type_(other.type_),
    value_(std::move(other.value_))
{
    other.type_  = Type::Null;
    other.value_ = nullptr;
}

Json& Json::operator=(Json&& other) noexcept {
    if (this != &other) {
        type_  = other.type_;
        value_ = std::move(other.value_);

        other.type_  = Type::Null;
        other.value_ = nullptr;
    }

    return *this;
}

// Parsing
Json Json::parse(std::string_view text) {
    Parser parser{ std::string(text) };

    return parser.parse();
}

// Type Inspection
Json::Type Json::type() const noexcept {
    return type_;
}

bool Json::isNull() const noexcept {
    return type_ == Type::Null;
}

bool Json::isBool() const noexcept {
    return type_ == Type::Bool;
}

bool Json::isNumber() const noexcept {
    return type_ == Type::Number;
}

bool Json::isString() const noexcept {
    return type_ == Type::String;
}

bool Json::isArray() const noexcept {
    return type_ == Type::Array;
}

bool Json::isObject() const noexcept {
    return type_ == Type::Object;
}

// Value Access
bool Json::asBool() const {
    const auto* p = std::get_if<bool>(&value_);

    if (!p)
        throw std::runtime_error("Json: not a bool");

    return *p;
}

double Json::asNumber() const {
    const auto* p = std::get_if<double>(&value_);

    if (!p)
        throw std::runtime_error("Json: not a number");

    return *p;
}

const std::string& Json::asString() const {
    const auto* p = std::get_if<std::string>(&value_);

    if (!p)
        throw std::runtime_error("Json: not a string");

    return *p;
}

Json::ArrayType& Json::asArray() {
    auto* p = std::get_if<ArrayType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an array");

    return *p;
}

const Json::ArrayType& Json::asArray() const {
    const auto* p = std::get_if<ArrayType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an array");

    return *p;
}

Json::ObjectType& Json::asObject() {
    auto* p = std::get_if<ObjectType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an object");

    return *p;
}

const Json::ObjectType& Json::asObject() const {
    const auto* p = std::get_if<ObjectType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an object");

    return *p;
}

// Navigation
Json& Json::operator[](std::size_t index) {
    auto* p = std::get_if<ArrayType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an object");

    return (*p)[index]; 
}

const Json& Json::operator[](std::size_t index) const {
    const auto* p = std::get_if<ArrayType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an array");

    return (*p)[index];
}

Json& Json::operator[](const std::string& key) {
    auto* p = std::get_if<ObjectType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an object");

    return (*p)[key];
}

const Json& Json::operator[](const std::string& key) const {
    const auto* p = std::get_if<ObjectType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an object");

    return p->find(key)->second; 
}

// Element Access
Json& Json::at(std::size_t index) {
    auto* p = std::get_if<ArrayType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an array");

    if (index >= p->size())
        throw std::out_of_range("Json::at: index out of range");

    return (*p)[index];
}

const Json& Json::at(std::size_t index) const {
    const auto* p = std::get_if<ArrayType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an array");

    if (index >= p->size())
        throw std::out_of_range("Json::at: index out of range");

    return (*p)[index];
}

Json& Json::at(const std::string& key) {
    auto* p = std::get_if<ObjectType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an object");

    auto it = p->find(key);

    if (it == p->end())
        throw std::out_of_range("Json::at: key not found");

    return it->second;
}

const Json& Json::at(const std::string& key) const {
    const auto* p = std::get_if<ObjectType>(&value_);

    if (!p)
        throw std::runtime_error("Json: not an object");

    auto it = p->find(key);

    if (it == p->end())
        throw std::out_of_range("Json::at: key not found");

    return it->second;
}

// Utilities
std::size_t Json::size() const noexcept {
    if (const auto* p = std::get_if<ArrayType>(&value_))
        return p->size();

    if (const auto* p = std::get_if<ObjectType>(&value_))
        return p->size();

    return 0;
}

bool Json::contains(const std::string& key) const noexcept {
    const auto* p = std::get_if<ObjectType>(&value_);

    if (!p)
        return false;

    return p->contains(key);
}

// Comparison
bool Json::operator==(const Json& other) const {
    if (type_ != other.type_)
        return false;

    switch (type_) {
        case Type::Null:   return true;
        case Type::Bool:   return std::get<bool>(value_)        == std::get<bool>(other.value_);
        case Type::Number: return std::get<double>(value_)      == std::get<double>(other.value_);
        case Type::String: return std::get<std::string>(value_) == std::get<std::string>(other.value_);
        case Type::Array:  return std::get<ArrayType>(value_)   == std::get<ArrayType>(other.value_);
        case Type::Object: return std::get<ObjectType>(value_)  == std::get<ObjectType>(other.value_);
    }

    return false;
}

bool Json::operator!=(const Json& other) const {
    return !(*this == other);
}

// Serialization
// dump(ostream) writes directly to the stream avoiding per-recursion string allocations
void Json::dump(std::ostream& os, int indent) const {
    switch (type_) {
        case Type::Null:
            os << "null";
            return;

        case Type::Bool:
            os << (std::get<bool>(value_) ? "true" : "false");
            return;

        case Type::Number:
            os << std::get<double>(value_);
            return;

        case Type::String:
            os << '"' << std::get<std::string>(value_) << '"';
            return;

        case Type::Array: {
            const auto& arr = std::get<ArrayType>(value_);

            if (arr.empty()) { os << "[]"; return; }

            os << "[\n";

            for (std::size_t i = 0; i < arr.size(); ++i) {
                os << pad(indent + 2);
                arr[i].dump(os, indent + 2);

                if (i + 1 != arr.size())
                    os << ',';

                os << '\n';
            }

            os << pad(indent) << ']';
            return;
        }

        case Type::Object: {
            const auto& obj = std::get<ObjectType>(value_);

            if (obj.empty()) { os << "{}"; return; }

            os << "{\n";

            bool first = true;

            for (const auto& [key, value] : obj) {
                if (!first)
                    os << ",\n";

                os << pad(indent + 2)
                   << '"' << key << "\": ";

                value.dump(os, indent + 2);

                first = false;
            }

            os << '\n' << pad(indent) << '}';
            return;
        }
    }
}

// dump(int) delegates to dump(ostream) to avoid string concatenation per recursion
std::string Json::dump(int indent) const {
    std::ostringstream oss;
    dump(oss, indent);
    return oss.str();
}
