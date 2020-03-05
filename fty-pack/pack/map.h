#pragma once
#include "pack/attribute.h"
#include "pack/node.h"
#include <algorithm>

namespace pack {

// ===========================================================================================================

class IMap : public Attribute
{
public:
    IMap(Attribute* parent, const std::string& key = {})
        : Attribute(NodeType::Map, parent, key)
    {
    }
};

template <typename T>
class Map : public IMap
{
public:
    using MapType       = std::vector<std::pair<std::string, T>>;
    using Iterator      = typename MapType::iterator;
    using ConstIterator = typename MapType::const_iterator;

public:
    using IMap::IMap;

public:
    ConstIterator begin() const;
    ConstIterator end() const;
    Iterator      begin();
    Iterator      end();

    const MapType& value() const;
    void           setValue(const MapType& val);
    bool           contains(const std::string& key) const;
    const T&       operator[](const std::string& key) const;
    int            size() const;
    Map&           operator=(const Map& other);
                   operator const T&() const;
    Map&           operator=(const MapType& val);

public:
    bool        compare(const Attribute& other) const override;
    std::string typeName() const override;
    void        set(const Attribute& other) override;
    void        set(Attribute&& other) override;
    bool        hasValue() const override;
    void        clear() override;

private:
    MapType m_value;
};

// ===========================================================================================================

template <typename T>
const typename Map<T>::MapType& Map<T>::value() const
{
    return m_value;
}

template <typename T>
typename Map<T>::ConstIterator Map<T>::begin() const
{
    return m_value.begin();
}

template <typename T>
typename Map<T>::ConstIterator Map<T>::end() const
{
    return m_value.end();
}

template <typename T>
typename Map<T>::Iterator Map<T>::begin()
{
    return m_value.begin();
}

template <typename T>
typename Map<T>::Iterator Map<T>::end()
{
    return m_value.end();
}

template <typename T>
int Map<T>::size() const
{
    return m_value.size();
}

template <typename T>
const T& Map<T>::operator[](const std::string& key) const
{
    auto found = std::find_if(m_value.begin(), m_value.end(), [&](const auto& pair) {
        return pair.first == key;
    });

    if (found) {
        return found->second;
    }

    throw std::out_of_range("Key " + key + " was not found");
}

// template <typename T>
// void Map<T>::load(const serialize::Node& node)
//{
//    for (const auto& it : node.children()) {
//        if constexpr (std::is_base_of_v<INode, T>) {
//            auto pair = std::make_pair<std::string, T>(
//                it.key(), {this, it.key()});
//            pair.second.load(it);
//            m_value.push_back(std::move(pair));
//        } else {
//            m_value.push_back({it.key(), it.as<T>()});
//        }
//    }
//}

// template <typename T>
// bool Map<T>::save(serialize::Node& node) const
//{
//    for (const auto& it : m_value) {
//        if constexpr (std::is_base_of_v<INode, T>) {
//            if (auto saved = it.second.save()) {
//                serialize::Node& child = node.addChild();
//                child.setKey(it.first);
//                child.set(*saved);
//            }
//        } else {
//            serialize::Node& child = node.addChild();
//            child.setKey(it.first);
//            child.set(it.second);
//        }
//    }
//    return node.children().size() > 0;
//}

template <typename T>
Map<T>& Map<T>::operator=(const Map& other)
{
    setValue(other.m_value);
    return *this;
}

template <typename T>
Map<T>::operator const T&() const
{
    return m_value;
}

template <typename T>
Map<T>& Map<T>::operator=(const MapType& val)
{
    setValue(val);
    return *this;
}

template <typename T>
void Map<T>::setValue(const MapType& val)
{
    if (m_value == val)
        return;

    m_parent->valueUpdated(*this);
    m_value = val;
}

template <typename T>
bool Map<T>::contains(const std::string& key) const
{
    return std::find_if(m_value.begin(), m_value.end(), [&](const auto& pair) {
        return pair.first == key;
    }) != m_value.end();
}

template <typename T>
bool Map<T>::compare(const Attribute& other) const
{
    if (auto casted = dynamic_cast<const Map<T>*>(&other)) {
        return casted->value() == value();
    }
    return false;
}

template <typename T>
std::string Map<T>::typeName() const
{
    return "Map";
}

template <typename T>
void Map<T>::set(const Attribute& other)
{
    if (auto casted = dynamic_cast<const Map<T>*>(&other)) {
        setValue(casted->value());
    }
}

template <typename T>
void Map<T>::set(Attribute&& other)
{
    if (auto casted = dynamic_cast<Map<T>*>(&other)) {
        setValue(std::move(casted->value()));
    }
}

template <typename T>
bool Map<T>::hasValue() const
{
    return !m_value.empty();
}

template <typename T>
void Map<T>::clear()
{
    m_value.clear();
}

// ===========================================================================================================

} // namespace pack
