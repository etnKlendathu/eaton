#pragma once
#include "pack/node.h"
#include <algorithm>

namespace pack {

// ===========================================================================================================

class IProtoMap : public Attribute
{
public:
    IProtoMap(Attribute* parent, const std::string& key = {})
        : Attribute(NodeType::Map, parent, key)
    {
    }

    virtual Node&       create()             = 0;
    virtual const Node& get(int index) const = 0;
    virtual int         size() const         = 0;
};

// ===========================================================================================================

template <typename KeyValue>
class ProtoMap : public IProtoMap
{
public:
    using MapType       = std::vector<KeyValue>;
    using Iterator      = typename MapType::iterator;
    using ConstIterator = typename MapType::const_iterator;
    using KeyType       = typename KeyValue::KeyType;
    using ValueType     = typename KeyValue::ValueType;

public:
    using IProtoMap::IProtoMap;

    ConstIterator begin() const;
    ConstIterator end() const;
    Iterator      begin();
    Iterator      end();

public:
    const MapType& value() const;
    void           setValue(const MapType& val);
    bool           contains(const KeyType& key) const;
    void           append(const KeyType& key, const ValueType& val);
    ValueType      operator[](const KeyType& key) const;

public:
    bool        compare(const Attribute& other) const override;
    std::string typeName() const override;
    void        set(const Attribute& other) override;
    void        set(Attribute&& other) override;
    bool        hasValue() const override;
    void        clear() override;
    Node&       create() override;
    int         size() const override;
    const Node& get(int index) const override;

private:
    MapType m_value;
};

// ===========================================================================================================

template <typename KeyValue>
typename ProtoMap<KeyValue>::ConstIterator ProtoMap<KeyValue>::begin() const
{
    return m_value.begin();
}

template <typename KeyValue>
typename ProtoMap<KeyValue>::ConstIterator ProtoMap<KeyValue>::end() const
{
    return m_value.end();
}

template <typename KeyValue>
typename ProtoMap<KeyValue>::Iterator ProtoMap<KeyValue>::begin()
{
    return m_value.begin();
}

template <typename KeyValue>
typename ProtoMap<KeyValue>::Iterator ProtoMap<KeyValue>::end()
{
    return m_value.end();
}

template <typename KeyValue>
bool ProtoMap<KeyValue>::compare(const Attribute& other) const
{
    if (auto casted = dynamic_cast<const ProtoMap<KeyValue>*>(&other)) {
        return casted->value() == value();
    }
    return false;
}

template <typename KeyValue>
std::string ProtoMap<KeyValue>::typeName() const
{
    return "ProtoMap";
}

template <typename KeyValue>
void ProtoMap<KeyValue>::set(const Attribute& other)
{
    if (auto casted = dynamic_cast<const ProtoMap<KeyValue>*>(&other)) {
        setValue(casted->value());
    }
}

template <typename KeyValue>
void ProtoMap<KeyValue>::set(Attribute&& other)
{
    if (auto casted = dynamic_cast<const ProtoMap<KeyValue>*>(&other)) {
        setValue(std::move(casted->value()));
    }
}

template <typename KeyValue>
bool ProtoMap<KeyValue>::hasValue() const
{
    return !m_value.empty();
}

template <typename KeyValue>
void ProtoMap<KeyValue>::clear()
{
    m_value.clear();
    m_parent->valueUpdated(*this);
}

template <typename KeyValue>
const typename ProtoMap<KeyValue>::MapType& ProtoMap<KeyValue>::value() const
{
    return m_value;
}

template <typename KeyValue>
void ProtoMap<KeyValue>::setValue(const MapType& val)
{
    m_value = val;
    m_parent->valueUpdated(*this);
}

template <typename KeyValue>
bool ProtoMap<KeyValue>::contains(const KeyType& key) const
{
    auto found = std::find_if(m_value.begin(), m_value.end(), [&](const KeyValue& val) {
        return val.key == key;
    });
    return found != m_value.end();
}

template <typename KeyValue>
void ProtoMap<KeyValue>::append(const KeyType& key, const ValueType& val)
{
    KeyValue it;
    it.key   = key;
    it.value = val;
    m_value.emplace_back(std::move(it));
    m_parent->valueUpdated(*this);
}

template <typename KeyValue>
typename ProtoMap<KeyValue>::ValueType ProtoMap<KeyValue>::operator[](const KeyType& key) const
{
    auto found = std::find_if(m_value.begin(), m_value.end(), [&](const KeyValue& val) {
        return val.key == key;
    });
    if (found != m_value.end()) {
        return found->value;
    }
    throw std::range_error("Not in range");
}

template <typename KeyValue>
int ProtoMap<KeyValue>::size() const
{
    return int(m_value.size());
}

template <typename KeyValue>
Node& ProtoMap<KeyValue>::create()
{
    KeyValue it;
    m_value.push_back(std::move(it));
    return m_value.back();
}

template <typename KeyValue>
const Node& ProtoMap<KeyValue>::get(int index) const
{
    return m_value[index];
}

// ===========================================================================================================

} // namespace pack
