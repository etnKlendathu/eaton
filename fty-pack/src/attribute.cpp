#include "pack/attribute.h"
#include <regex>

//pack::Attribute::Attribute()
//{
//}

pack::Attribute::Attribute(NodeType type, Attribute* parent, const std::string& key)
    : m_parent(parent)
    , m_key(key)
    , m_type(type)
{
}

const std::string& pack::Attribute::key() const
{
    return m_key;
}

void pack::Attribute::valueUpdated(const Attribute& /*attr*/)
{
    if (m_parent) {
        m_parent->valueUpdated(*this);
    }
}

bool pack::Attribute::operator==(const pack::Attribute& other) const
{
    return compare(other);
}

bool pack::Attribute::operator!=(const pack::Attribute& other) const
{
    return !compare(other);
}

const pack::Attribute* pack::Attribute::parent() const
{
    return m_parent;
}

pack::Attribute::NodeType pack::Attribute::type() const
{
    return m_type;
}

std::vector<std::string> pack::split(const std::string& str)
{
    static std::regex rgx("\\s+");

    std::vector<std::string> ret;
    std::sregex_token_iterator iter(str.begin(), str.end(), rgx, -1);
    std::sregex_token_iterator end;
    for ( ; iter != end; ++iter)
        ret.push_back(*iter);
    return ret;
}
