#pragma once

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <unordered_map>

//#include "VectorPro.h"
//#include "HashMap.h"

class Json {
public:

	// Type
	enum class Type {
		Null,
		Bool,
		Number,
		String,
		Array,
		Object
	};

	// Aliases
#ifdef JSON_USE_CUSTOM
	//using ArrayType  = VectorPro<Json>;
	//using ObjectType = HashMap<std::string, Json>;
#else
	using ArrayType  = std::vector<Json>;
	using ObjectType = std::unordered_map<std::string, Json>;
#endif
private:

	// Core State
	Type type_;

	std::variant<
	std::nullptr_t,
	    bool,
	    double,
	    std::string,
	    ArrayType,
	    ObjectType
	    > value_;

public:

	// Lifecycle
	Json();
	Json(std::nullptr_t);

	Json(bool value);

	Json(double value);
	Json(int value);

	Json(const std::string& value);
	Json(std::string&& value);
	Json(const char* value);

	Json(const ArrayType& value);
	Json(ArrayType&& value);

	Json(const ObjectType& value);
	Json(ObjectType&& value);

	Json(const Json& other);
	Json& operator=(const Json& other);

	Json(Json&& other) noexcept;
	Json& operator=(Json&& other) noexcept;

	// Parsing
	[[nodiscard]] static Json parse(std::string_view text);

	// Type Inspection
	[[nodiscard]] Type type()     const noexcept;
	[[nodiscard]] bool isNull()   const noexcept;
	[[nodiscard]] bool isBool()   const noexcept;
	[[nodiscard]] bool isNumber() const noexcept;
	[[nodiscard]] bool isString() const noexcept;
	[[nodiscard]] bool isArray()  const noexcept;
	[[nodiscard]] bool isObject() const noexcept;

	// Value Access
	[[nodiscard]] bool                asBool()   const;
	[[nodiscard]] double              asNumber() const;
	[[nodiscard]] const std::string&  asString() const;

	[[nodiscard]] ArrayType&          asArray();
	[[nodiscard]] const ArrayType&    asArray()  const;

	[[nodiscard]] ObjectType&         asObject();
	[[nodiscard]] const ObjectType&   asObject() const;

	// Navigation
	[[nodiscard]] Json&       operator[](const std::string& key);
	[[nodiscard]] const Json& operator[](const std::string& key) const;

	[[nodiscard]] Json&       operator[](std::size_t index);
	[[nodiscard]] const Json& operator[](std::size_t index) const;

	// Element Access
	[[nodiscard]] Json& at(std::size_t index);
	[[nodiscard]] const Json& at(std::size_t index) const;

	[[nodiscard]] Json& at(const std::string& key);
	[[nodiscard]] const Json& at(const std::string& key) const;

	// Utilities
	[[nodiscard]] std::size_t size() const noexcept;
	[[nodiscard]] bool contains(const std::string& key) const noexcept;

	// Comparison
	[[nodiscard]] bool operator==(const Json& other) const;
	[[nodiscard]] bool operator!=(const Json& other) const;

	// Serialization
	[[nodiscard]] std::string dump(int indent = 0) const;
	void dump(std::ostream& os, int indent = 0) const;
};

