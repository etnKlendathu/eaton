#pragma once
#include <string>
#include "pack/attribute.h"
#include "pack/node.h"
#include "pack/magic_enum.h"
#include <sstream>

namespace pack {

enum class Type
{
    Unknown,
    String,
    Int32,
    Int64,
    UInt32,
    UInt64,
    Float,
    Double,
    Bool
};

template<Type>
struct ResolveType;

template<> struct ResolveType<Type::String> { using type = std::string; };
template<> struct ResolveType<Type::Int32> { using type = int32_t; };
template<> struct ResolveType<Type::Int64> { using type = int64_t; };
template<> struct ResolveType<Type::UInt32> { using type = uint32_t; };
template<> struct ResolveType<Type::UInt64> { using type = uint64_t; };
template<> struct ResolveType<Type::Float> { using type = float; };
template<> struct ResolveType<Type::Double> { using type = double; };
template<> struct ResolveType<Type::Bool> { using type = bool; };

inline std::string valueTypeName(Type type)
{
    using namespace magic_enum::ostream_operators;
    std::stringstream ss;
    ss << type;
    return ss.str();
}

}
