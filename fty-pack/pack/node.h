#pragma once
#include "pack/attribute.h"

namespace pack {

// ===========================================================================================================

/// Node interface
class INode : public Attribute
{
public:
    INode()
        : Attribute(NodeType::Node, nullptr)
    {
    }

    INode(Attribute* parent, const std::string& key = {})
        : Attribute(NodeType::Node, parent, key)
    {
    }

    /// Dumps a class as yaml serialized string
    virtual std::string dump() const = 0;

    /// Returns a list of fields
    virtual std::vector<Attribute*> fields() = 0;

    /// Returns a list of fields
    virtual std::vector<const Attribute*> fields() const = 0;

    /// Returns a list of the fields names
    virtual std::vector<std::string> fieldNames() const = 0;

    virtual const std::string& fileDescriptor() const = 0;

    virtual std::string protoName() const = 0;
};

/// Node class
///
/// This class used as main holder of the structured data.
/// Typical usage is:
/// ---------------------------
/// struct MyClass: public pack::Node {
///     using pack::Node::Node;
///     pack::String str = FIELD("string-value", 1)
///     META(MyClass, str)
/// }
/// ---------------------------
/// Where each field has a usage name (str) and real name used to serialization/deserialization (string-value)
/// META macro just define metainformation about a struct, such as struct name and list of the fields.
class Node : public INode
{
public:
    using INode::INode;

    /// Compares classes by content
    bool compare(const Attribute& other) const override;

    /// Dumps a class as yaml serialized string
    std::string dump() const override;

    /// Returns field it's key
    const Attribute* fieldByKey(const std::string& name) const;

    /// Returns field it's name
    const Attribute* fieldByName(const std::string& name) const;

    void set(const Attribute& other) override;
    void set(Attribute&& other) override;

    /// Checks if this class has any nondefault field
    bool hasValue() const override;

    const std::string& fileDescriptor() const override;

    std::string protoName() const override;

    void clear() override;
};

// ===========================================================================================================

} // namespace pack
