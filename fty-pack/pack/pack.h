#pragma once
#include "pack/enum.h"
#include "pack/list.h"
#include "pack/map.h"
#include "pack/node.h"
#include "pack/value.h"
#include "pack/serialization.h"

namespace pack {

using Int32  = Value<Type::Int32>;
using Int64  = Value<Type::Int64>;
using UInt32 = Value<Type::UInt32>;
using UInt64 = Value<Type::UInt64>;
using Float  = Value<Type::Float>;
using Double = Value<Type::Double>;
using Bool   = Value<Type::Bool>;

//============================================================================================================

class String : public Value<Type::String>
{
public:
    using pack::Value<Type::String>::Value;
    using pack::Value<Type::String>::operator=;

    bool empty() const
    {
        return value().empty();
    }

    int size() const
    {
        return int(value().size());
    }
};

template <typename T>
inline bool operator==(const String& l, const T& r)
{
    return l.value() == r;
}

template <typename T>
inline bool operator==(const T& l, const String& r)
{
    return r.value() == l;
}

} // namespace pack
