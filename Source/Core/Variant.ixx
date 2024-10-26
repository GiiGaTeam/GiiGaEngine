module;

#include <optional>
#include <string>
#include <variant>

export module Variant;

export struct ComponentName {
    std::string name;
};

export class Variant
{
public:
    struct Empty {};
    
    using ValueType = std::variant<
        Empty,
        bool,
        int64_t,
        float,
        std::string,
        ComponentName
    >;

    Variant() : value_(Empty{}) {}
    Variant(bool b) : value_(b) {}
    Variant(int64_t i) : value_(i) {}
    Variant(float f) : value_(f) {}
    Variant(std::string&& str) : value_(std::move(str)) {}
    Variant(ComponentName componentName) : value_(std::move(componentName)) {}
    
    bool IsEmpty() const {
        return std::holds_alternative<Empty>(value_);
    }

    std::optional<std::reference_wrapper<bool>> GetBool() {
        return GetInternal<bool>();
    }

    std::optional<std::reference_wrapper<int64_t>> GetInt() {
        return GetInternal<int64_t>();
    }

    std::optional<std::reference_wrapper<float>> GetFloat() {
        return GetInternal<float>();
    }

    std::optional<std::reference_wrapper<std::string>> GetString() {
        return GetInternal<std::string>();
    }

    std::optional<std::reference_wrapper<ComponentName>> GetComponentName() {
        return GetInternal<ComponentName>();
    }
private:
    template <typename T>
    std::optional<std::reference_wrapper<T>> GetInternal() {
        if (auto ptr = std::get_if<T>(&value_)) {
            return std::ref(*ptr);
        }
        return std::nullopt;
    }

    template <typename T>
    std::optional<std::reference_wrapper<const T>> GetInternal() const {
        if (auto ptr = std::get_if<T>(&value_)) {
            return std::cref(*ptr);
        }
        return std::nullopt;
    }
    
    ValueType value_;

};