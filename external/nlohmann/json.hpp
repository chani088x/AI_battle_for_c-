#pragma once

#include <cctype>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <sstream>

namespace nlohmann {

class json {
public:
    using object_t = std::map<std::string, json>;
    using array_t = std::vector<json>;
    using string_t = std::string;
    using boolean_t = bool;
    using number_t = double;
    using variant_t = std::variant<std::nullptr_t, boolean_t, number_t, string_t, object_t, array_t>;

    json() : data_(nullptr) {}
    json(std::nullptr_t) : data_(nullptr) {}
    json(boolean_t value) : data_(value) {}
    json(int value) : data_(static_cast<number_t>(value)) {}
    json(long long value) : data_(static_cast<number_t>(value)) {}
    json(number_t value) : data_(value) {}
    json(const char* value) : data_(string_t(value)) {}
    json(const string_t& value) : data_(value) {}
    json(string_t&& value) : data_(std::move(value)) {}
    json(const object_t& value) : data_(value) {}
    json(object_t&& value) : data_(std::move(value)) {}
    json(const array_t& value) : data_(value) {}
    json(array_t&& value) : data_(std::move(value)) {}

    static json object() {
        return json(object_t{});
    }

    static json array() {
        return json(array_t{});
    }

    bool is_null() const { return std::holds_alternative<std::nullptr_t>(data_); }
    bool is_boolean() const { return std::holds_alternative<boolean_t>(data_); }
    bool is_number() const { return std::holds_alternative<number_t>(data_); }
    bool is_string() const { return std::holds_alternative<string_t>(data_); }
    bool is_object() const { return std::holds_alternative<object_t>(data_); }
    bool is_array() const { return std::holds_alternative<array_t>(data_); }

    json& operator[](const std::string& key) {
        if (!is_object()) {
            data_ = object_t{};
        }
        return std::get<object_t>(data_)[key];
    }

    const json& operator[](const std::string& key) const {
        static json null_json;
        if (!is_object()) {
            return null_json;
        }
        auto it = std::get<object_t>(data_).find(key);
        if (it == std::get<object_t>(data_).end()) {
            return null_json;
        }
        return it->second;
    }

    json& operator[](size_t index) {
        if (!is_array()) {
            throw std::runtime_error("json value is not an array");
        }
        auto& arr = std::get<array_t>(data_);
        if (index >= arr.size()) {
            throw std::out_of_range("json index out of range");
        }
        return arr[index];
    }

    const json& operator[](size_t index) const {
        if (!is_array()) {
            throw std::runtime_error("json value is not an array");
        }
        const auto& arr = std::get<array_t>(data_);
        if (index >= arr.size()) {
            throw std::out_of_range("json index out of range");
        }
        return arr[index];
    }

    void push_back(const json& value) {
        if (!is_array()) {
            data_ = array_t{};
        }
        std::get<array_t>(data_).push_back(value);
    }

    size_t size() const {
        if (is_array()) {
            return std::get<array_t>(data_).size();
        }
        if (is_object()) {
            return std::get<object_t>(data_).size();
        }
        return 0;
    }

    bool contains(const std::string& key) const {
        if (!is_object()) {
            return false;
        }
        const auto& obj = std::get<object_t>(data_);
        return obj.find(key) != obj.end();
    }

    template <typename T>
    T get() const {
        return get_impl<T>();
    }

    template <typename T>
    T value(const std::string& key, const T& default_value) const {
        if (!is_object()) {
            return default_value;
        }
        const auto& obj = std::get<object_t>(data_);
        auto it = obj.find(key);
        if (it == obj.end()) {
            return default_value;
        }
        return it->second.template get<T>();
    }

    std::string dump(int indent = -1) const {
        std::ostringstream oss;
        dump_impl(oss, *this, indent, 0);
        return oss.str();
    }

    static json parse(const std::string& input) {
        size_t index = 0;
        json result = parse_value(input, index);
        skip_ws(input, index);
        if (index != input.size()) {
            throw std::runtime_error("unexpected trailing characters in json");
        }
        return result;
    }

private:
    variant_t data_;

    template <typename T>
    T get_impl() const;

    static void dump_indent(std::ostringstream& oss, int indent, int depth) {
        if (indent >= 0) {
            oss << '\n';
            for (int i = 0; i < depth * indent; ++i) {
                oss << ' ';
            }
        }
    }

    static void dump_impl(std::ostringstream& oss, const json& value, int indent, int depth) {
        if (value.is_null()) {
            oss << "null";
            return;
        }
        if (value.is_boolean()) {
            oss << (std::get<boolean_t>(value.data_) ? "true" : "false");
            return;
        }
        if (value.is_number()) {
            number_t num = std::get<number_t>(value.data_);
            if (std::floor(num) == num) {
                oss << static_cast<long long>(num);
            } else {
                oss << num;
            }
            return;
        }
        if (value.is_string()) {
            oss << '"' << escape_string(std::get<string_t>(value.data_)) << '"';
            return;
        }
        if (value.is_array()) {
            const auto& arr = std::get<array_t>(value.data_);
            oss << '[';
            if (!arr.empty()) {
                for (size_t i = 0; i < arr.size(); ++i) {
                    if (i > 0) {
                        oss << ',';
                    }
                    dump_indent(oss, indent, depth + 1);
                    dump_impl(oss, arr[i], indent, depth + 1);
                }
                dump_indent(oss, indent, depth);
            }
            oss << ']';
            return;
        }
        if (value.is_object()) {
            const auto& obj = std::get<object_t>(value.data_);
            oss << '{';
            bool first = true;
            for (const auto& [key, val] : obj) {
                if (!first) {
                    oss << ',';
                }
                first = false;
                dump_indent(oss, indent, depth + 1);
                oss << '"' << escape_string(key) << '"' << ':';
                if (indent >= 0) {
                    oss << ' ';
                }
                dump_impl(oss, val, indent, depth + 1);
            }
            if (!obj.empty()) {
                dump_indent(oss, indent, depth);
            }
            oss << '}';
        }
    }

    static std::string escape_string(const std::string& input) {
        std::string result;
        for (char c : input) {
            switch (c) {
                case '\\': result += "\\\\"; break;
                case '"': result += "\\\""; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }

    static void skip_ws(const std::string& input, size_t& index) {
        while (index < input.size() && std::isspace(static_cast<unsigned char>(input[index]))) {
            ++index;
        }
    }

    static json parse_value(const std::string& input, size_t& index) {
        skip_ws(input, index);
        if (index >= input.size()) {
            throw std::runtime_error("unexpected end of input");
        }
        char c = input[index];
        if (c == 'n') {
            expect(input, index, "null");
            return json(nullptr);
        }
        if (c == 't') {
            expect(input, index, "true");
            return json(true);
        }
        if (c == 'f') {
            expect(input, index, "false");
            return json(false);
        }
        if (c == '"') {
            return json(parse_string(input, index));
        }
        if (c == '[') {
            return parse_array(input, index);
        }
        if (c == '{') {
            return parse_object(input, index);
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return json(parse_number(input, index));
        }
        throw std::runtime_error("invalid json value");
    }

    static void expect(const std::string& input, size_t& index, const std::string& expected) {
        if (input.compare(index, expected.size(), expected) != 0) {
            throw std::runtime_error("unexpected token in json");
        }
        index += expected.size();
    }

    static std::string parse_string(const std::string& input, size_t& index) {
        if (input[index] != '"') {
            throw std::runtime_error("expected string");
        }
        ++index;
        std::string result;
        while (index < input.size()) {
            char c = input[index++];
            if (c == '"') {
                break;
            }
            if (c == '\\') {
                if (index >= input.size()) {
                    throw std::runtime_error("invalid escape sequence");
                }
                char esc = input[index++];
                switch (esc) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: throw std::runtime_error("unsupported escape sequence");
                }
            } else {
                result += c;
            }
        }
        return result;
    }

    static number_t parse_number(const std::string& input, size_t& index) {
        size_t start = index;
        if (input[index] == '-') {
            ++index;
        }
        while (index < input.size() && std::isdigit(static_cast<unsigned char>(input[index]))) {
            ++index;
        }
        if (index < input.size() && input[index] == '.') {
            ++index;
            while (index < input.size() && std::isdigit(static_cast<unsigned char>(input[index]))) {
                ++index;
            }
        }
        if (index < input.size() && (input[index] == 'e' || input[index] == 'E')) {
            ++index;
            if (index < input.size() && (input[index] == '+' || input[index] == '-')) {
                ++index;
            }
            while (index < input.size() && std::isdigit(static_cast<unsigned char>(input[index]))) {
                ++index;
            }
        }
        return std::stod(input.substr(start, index - start));
    }

    static json parse_array(const std::string& input, size_t& index) {
        if (input[index] != '[') {
            throw std::runtime_error("expected array");
        }
        ++index;
        json result = json::array();
        skip_ws(input, index);
        if (index < input.size() && input[index] == ']') {
            ++index;
            return result;
        }
        while (index < input.size()) {
            result.push_back(parse_value(input, index));
            skip_ws(input, index);
            if (index >= input.size()) {
                throw std::runtime_error("unexpected end of input in array");
            }
            if (input[index] == ',') {
                ++index;
                continue;
            }
            if (input[index] == ']') {
                ++index;
                break;
            }
            throw std::runtime_error("expected ',' or ']' in array");
        }
        return result;
    }

    static json parse_object(const std::string& input, size_t& index) {
        if (input[index] != '{') {
            throw std::runtime_error("expected object");
        }
        ++index;
        json result = json::object();
        skip_ws(input, index);
        if (index < input.size() && input[index] == '}') {
            ++index;
            return result;
        }
        while (index < input.size()) {
            skip_ws(input, index);
            std::string key = parse_string(input, index);
            skip_ws(input, index);
            if (index >= input.size() || input[index] != ':') {
                throw std::runtime_error("expected ':' in object");
            }
            ++index;
            result[key] = parse_value(input, index);
            skip_ws(input, index);
            if (index >= input.size()) {
                throw std::runtime_error("unexpected end of input in object");
            }
            if (input[index] == ',') {
                ++index;
                continue;
            }
            if (input[index] == '}') {
                ++index;
                break;
            }
            throw std::runtime_error("expected ',' or '}' in object");
        }
        return result;
    }
};

// Specializations

template <>
inline std::string json::get<std::string>() const {
    if (!is_string()) {
        throw std::runtime_error("json value is not a string");
    }
    return std::get<string_t>(data_);
}

template <>
inline int json::get<int>() const {
    if (is_number()) {
        return static_cast<int>(std::get<number_t>(data_));
    }
    throw std::runtime_error("json value is not a number");
}

template <>
inline double json::get<double>() const {
    if (is_number()) {
        return std::get<number_t>(data_);
    }
    throw std::runtime_error("json value is not a number");
}

template <>
inline bool json::get<bool>() const {
    if (is_boolean()) {
        return std::get<boolean_t>(data_);
    }
    throw std::runtime_error("json value is not a boolean");
}

template <>
inline json::array_t json::get<json::array_t>() const {
    if (!is_array()) {
        throw std::runtime_error("json value is not an array");
    }
    return std::get<array_t>(data_);
}

template <>
inline json::object_t json::get<json::object_t>() const {
    if (!is_object()) {
        throw std::runtime_error("json value is not an object");
    }
    return std::get<object_t>(data_);
}

} // namespace nlohmann
