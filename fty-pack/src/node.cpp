#include "pack/node.h"
#include "pack/serialization.h"
#include <algorithm>

std::string pack::Node::dump() const
{
    return yaml::serialize(*this);
}


const pack::Attribute* pack::Node::fieldByKey(const std::string& key) const
{
    const auto flds = fields();
    auto       it   = std::find_if(flds.begin(), flds.end(), [&](const auto* attr) {
        return attr->key() == key;
    });
    return it != fields().end() ? *it : nullptr;
}

const pack::Attribute* pack::Node::fieldByName(const std::string& name) const
{
    auto names = fieldNames();
    auto it    = std::find(names.begin(), names.end(), name);
    if (it != names.end()) {
        return fields()[size_t(std::distance(names.begin(), it))];
    }
    return nullptr;
}

bool pack::Node::compare(const pack::Attribute& other) const
{
    if (auto casted = dynamic_cast<const Node*>(&other)) {
        for (const auto& it : fields()) {
            const Attribute* ofield = casted->fieldByKey(it->key());
            if (!ofield) {
                return false;
            }

            if (*it != *ofield) {
                return false;
            }
        }
        return true;
    }
    return false;
}

// std::string pack::Node::typeName() const
//{
//    return "Node";
//}

void pack::Node::set(const Attribute& other)
{
    if (auto casted = dynamic_cast<const Node*>(&other)) {
        for (const auto& it : fields()) {
            const Attribute* ofield = casted->fieldByKey(it->key());
            if (ofield) {
                it->set(*ofield);
            }
        }
    }
}

void pack::Node::set(Attribute&& other)
{
    if (auto casted = dynamic_cast<Node*>(&other)) {
        for (const auto& it : fields()) {
            const Attribute* ofield = casted->fieldByKey(it->key());
            if (ofield) {
                it->set(std::move(*ofield));
            }
        }
    }
}

bool pack::Node::hasValue() const
{
    for (const auto& it : fields()) {
        if (it->hasValue()) {
            return true;
        }
    }
    return false;
}

const std::string& pack::Node::fileDescriptor() const
{
    static std::string desc;
    return desc;
}

std::string pack::Node::protoName() const
{
    return typeName();
}

void pack::Node::clear()
{
    for (auto& it : fields()) {
        it->clear();
    }
}
