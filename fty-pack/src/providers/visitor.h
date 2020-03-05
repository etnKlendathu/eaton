#pragma once
#include "pack/pack.h"

namespace pack {

template<typename Worker>
class Deserialize
{
public:

    template<typename Resource>
    static void visit(INode& node, const Resource& res)
    {
        Worker::unpackValue(node, res);
    }

    template<typename Resource>
    static void visit(IEnum& en, const Resource& res)
    {
        Worker::unpackValue(en, res);
    }

    template<typename Resource>
    static void visit(IList& list, const Resource& res)
    {
        if (auto casted = dynamic_cast<IObjectList*>(&list)) {
            Worker::unpackValue(*casted, res);
        }

        if (auto casted = dynamic_cast<IValueList*>(&list)) {
            switch (casted->valueType()) {
            case Type::Bool:
                Worker::unpackValue(static_cast<ValueList<Type::Bool>&>(list), res);
                break;
            case Type::Double:
                Worker::unpackValue(static_cast<ValueList<Type::Double>&>(list), res);
                break;
            case Type::Float:
                Worker::unpackValue(static_cast<ValueList<Type::Float>&>(list), res);
                break;
            case Type::String:
                Worker::unpackValue(static_cast<ValueList<Type::String>&>(list), res);
                break;
            case Type::Int32:
                Worker::unpackValue(static_cast<ValueList<Type::Int32>&>(list), res);
                break;
            case Type::UInt32:
                Worker::unpackValue(static_cast<ValueList<Type::UInt32>&>(list), res);
                break;
            case Type::Int64:
                Worker::unpackValue(static_cast<ValueList<Type::Int64>&>(list), res);
                break;
            case Type::UInt64:
                Worker::unpackValue(static_cast<ValueList<Type::UInt64>&>(list), res);
                break;
            default:
                throw std::runtime_error("Unsupported type to unpack");
            }
        }
    }

    template<typename Resource>
    static void visit(IMap& /*map*/, const Resource& /*res*/)
    {
    }

    template<typename Resource>
    static void visit(IValue& value, const Resource& res)
    {
        switch (value.valueType()) {
        case Type::Bool:
            Worker::unpackValue(static_cast<Bool&>(value), res);
            break;
        case Type::Double:
            Worker::unpackValue(static_cast<Double&>(value), res);
            break;
        case Type::Float:
            Worker::unpackValue(static_cast<Float&>(value), res);
            break;
        case Type::String:
            Worker::unpackValue(static_cast<String&>(value), res);
            break;
        case Type::Int32:
            Worker::unpackValue(static_cast<Int32&>(value), res);
            break;
        case Type::UInt32:
            Worker::unpackValue(static_cast<UInt32&>(value), res);
            break;
        case Type::Int64:
            Worker::unpackValue(static_cast<Int64&>(value), res);
            break;
        case Type::UInt64:
            Worker::unpackValue(static_cast<UInt64&>(value), res);
            break;
        default:
            throw std::runtime_error("Unsupported type to unpack");
        }
    }

    template<typename Resource>
    static void visit(Attribute& node, const Resource& res)
    {
        switch (node.type()) {
        case Attribute::NodeType::Enum:
            visit(static_cast<IEnum&>(node), res);
            break;
        case Attribute::NodeType::List:
            visit(static_cast<IList&>(node), res);
            break;
        case Attribute::NodeType::Map:
            visit(static_cast<IMap&>(node), res);
            break;
        case Attribute::NodeType::Node:
            visit(static_cast<INode&>(node), res);
            break;
        case Attribute::NodeType::Value:
            visit(static_cast<IValue&>(node), res);
            break;
        }
    }
};

template<typename Worker>
class Serialize
{
public:

    template<typename Resource>
    static void visit(const INode& node, Resource& res)
    {
        Worker::packValue(node, res);
    }

    template<typename Resource>
    static void visit(const IEnum& en, Resource& res)
    {
        Worker::packValue(en, res);
    }

    template<typename Resource>
    static void visit(const IList& list, Resource& res)
    {
        if (auto casted = dynamic_cast<const IObjectList*>(&list)) {
            Worker::packValue(*casted, res);
        }

        if (auto casted = dynamic_cast<const IValueList*>(&list)) {
            switch (casted->valueType()) {
            case Type::Bool:
                Worker::packValue(static_cast<const ValueList<Type::Bool>&>(list), res);
                break;
            case Type::Double:
                Worker::packValue(static_cast<const ValueList<Type::Double>&>(list), res);
                break;
            case Type::Float:
                Worker::packValue(static_cast<const ValueList<Type::Float>&>(list), res);
                break;
            case Type::String:
                Worker::packValue(static_cast<const ValueList<Type::String>&>(list), res);
                break;
            case Type::Int32:
                Worker::packValue(static_cast<const ValueList<Type::Int32>&>(list), res);
                break;
            case Type::UInt32:
                Worker::packValue(static_cast<const ValueList<Type::UInt32>&>(list), res);
                break;
            case Type::Int64:
                Worker::packValue(static_cast<const ValueList<Type::Int64>&>(list), res);
                break;
            case Type::UInt64:
                Worker::packValue(static_cast<const ValueList<Type::UInt64>&>(list), res);
                break;
            default:
                throw std::runtime_error("Unsupported type to unpack");
            }
        }
    }

    template<typename Resource>
    static void visit(const IMap& /*map*/, Resource& /*res*/)
    {
    }

    template<typename Resource>
    static void visit(const IValue& value, Resource& res)
    {
        switch (value.valueType()) {
        case Type::Bool:
            Worker::packValue(static_cast<const Bool&>(value), res);
            break;
        case Type::Double:
            Worker::packValue(static_cast<const Double&>(value), res);
            break;
        case Type::Float:
            Worker::packValue(static_cast<const Float&>(value), res);
            break;
        case Type::String:
            Worker::packValue(static_cast<const String&>(value), res);
            break;
        case Type::Int32:
            Worker::packValue(static_cast<const Int32&>(value), res);
            break;
        case Type::UInt32:
            Worker::packValue(static_cast<const UInt32&>(value), res);
            break;
        case Type::Int64:
            Worker::packValue(static_cast<const Int64&>(value), res);
            break;
        case Type::UInt64:
            Worker::packValue(static_cast<const UInt64&>(value), res);
            break;
        default:
            throw std::runtime_error("Unsupported type to unpack");
        }
    }

    template<typename Resource>
    static void visit(const Attribute& node, Resource& res)
    {
        switch (node.type()) {
        case Attribute::NodeType::Enum:
            visit(static_cast<const IEnum&>(node), res);
            break;
        case Attribute::NodeType::List:
            visit(static_cast<const IList&>(node), res);
            break;
        case Attribute::NodeType::Map:
            visit(static_cast<const IMap&>(node), res);
            break;
        case Attribute::NodeType::Node:
            visit(static_cast<const INode&>(node), res);
            break;
        case Attribute::NodeType::Value:
            visit(static_cast<const IValue&>(node), res);
            break;
        }
    }
};

}
