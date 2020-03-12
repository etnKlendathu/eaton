#pragma once
#include <string>
#include <type_traits>
#include <sstream>
#include "traits.h"

namespace fty {

template<typename T, typename VT>
std::enable_if_t<std::is_constructible_v<std::string, VT>, T> convert(const VT& value)
{
    std::string string(value);
    if (string.empty()) {
        return T{};
    }
    if constexpr (std::is_same_v<T, std::string>) {
        return string;
    } else if constexpr (std::is_same_v<T, bool>) {
        return string == "1" || string == "true";
    } else if constexpr (std::is_same_v<T, float>) {
        return std::stof(string);
    } else if constexpr (std::is_same_v<T, double>) {
        return std::stod(string);
    } else if constexpr (std::is_same_v<T, int32_t>) {
        return std::stoi(string);
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        return uint32_t(std::stoul(string));
    } else if constexpr (std::is_same_v<T, int64_t>) {
        return std::stoll(string);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        return std::stoull(string);
    } else {
        static_assert(fty::always_false<T>, "Unsupported type");
    }
}

template<typename T, typename VT>
std::enable_if_t<std::is_integral_v<VT> && !std::is_same_v<bool, VT>, T> convert(const VT& value)
{
    if constexpr (std::is_constructible_v<std::string, T>) {
        return std::to_string(value);
    } else {
        return static_cast<T>(value);
    }
}

template<typename T, typename VT>
std::enable_if_t<std::is_floating_point_v<VT>, T> convert(const VT& value)
{
    if constexpr (std::is_constructible_v<std::string, T>) {
        // In case of floats std::to_string gives not pretty results. So, just use stringstream here.
        std::stringstream ss;
        ss << value;
        return ss.str();
    } else {
        return static_cast<T>(value);
    }
}

template<typename T>
T convert(bool value)
{
    if constexpr (std::is_constructible_v<std::string, T>) {
        return value ? "true" : "false";
    } else {
        return static_cast<T>(value);
    }
}

} // namespace fty
