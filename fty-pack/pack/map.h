#pragma once
#include "pack/attribute.h"
#include "pack/node.h"
#include "pack/types.h"
#include <algorithm>
#include <map>

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

// ===========================================================================================================

/// Object map interface.
///
/// This container is used to keep Node types
class IObjectMap : public IMap
{
public:
    using IMap::IMap;

public:
    /// Returns INode interface by index
    virtual const INode& get(const std::string& key) const = 0;
    virtual INode&       create()                          = 0;
};

// ===========================================================================================================

/// Values list interface.
///
/// This container is used to keep values simple types
class IValueMap : public IMap
{
public:
    using IMap::IMap;

public:
    /// Returns values type
    virtual Type valueType() const = 0;
};

// ===========================================================================================================

template <typename T>
class Map : public IObjectMap
{
public:
    using MapType       = std::vector<std::pair<std::string, T>>;
    using Iterator      = typename MapType::iterator;
    using ConstIterator = typename MapType::const_iterator;

public:
    using IObjectMap::IObjectMap;

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
                   operator const T &() const;
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

template <Type ValType>
class ValueMap : public IValueMap
{
public:
    static constexpr Type ThisType = ValType;

    using CppType       = typename ResolveType<ValType>::type;
    using MapType       = std::map<std::string, CppType>;
    using Iterator      = typename MapType::iterator;
    using ConstIterator = typename MapType::const_iterator;

public:
    using IValueMap::IValueMap;

public:
    ConstIterator begin() const;
    ConstIterator end() const;
    Iterator      begin();
    Iterator      end();

    const MapType& value() const;
    void           setValue(const MapType& val);
    bool           contains(const std::string& key) const;
    const CppType& operator[](const std::string& key) const;
    int            size() const;
    ValueMap&      operator=(const ValueMap& other);
    ValueMap&      operator=(const MapType& val);
    void           append(const std::string& key, const CppType& val);

public:
    bool        compare(const Attribute& other) const override;
    std::string typeName() const override;
    void        set(const Attribute& other) override;
    void        set(Attribute&& other) override;
    bool        hasValue() const override;
    void        clear() override;
    Type        valueType() const override;

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

template <typename T>
Map<T>& Map<T>::operator=(const Map& other)
{
    setValue(other.m_value);
    return *this;
}

template <typename T>
Map<T>::operator const T &() const
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

template <Type ValType>
const typename ValueMap<ValType>::MapType& ValueMap<ValType>::value() const
{
    return m_value;
}

template <Type ValType>
typename ValueMap<ValType>::ConstIterator ValueMap<ValType>::begin() const
{
    return m_value.begin();
}

template <Type ValType>
typename ValueMap<ValType>::ConstIterator ValueMap<ValType>::end() const
{
    return m_value.end();
}

template <Type ValType>
typename ValueMap<ValType>::Iterator ValueMap<ValType>::begin()
{
    return m_value.begin();
}

template <Type ValType>
typename ValueMap<ValType>::Iterator ValueMap<ValType>::end()
{
    return m_value.end();
}

template <Type ValType>
int ValueMap<ValType>::size() const
{
    return m_value.size();
}

template <Type ValType>
const typename ValueMap<ValType>::CppType& ValueMap<ValType>::operator[](const std::string& key) const
{
    auto found = m_value.find(key);
    if (found != m_value.end()) {
        return found->second;
    }

    throw std::out_of_range("Key " + key + " was not found");
}

template <Type ValType>
ValueMap<ValType>& ValueMap<ValType>::operator=(const ValueMap& other)
{
    setValue(other.m_value);
    return *this;
}

template <Type ValType>
ValueMap<ValType>& ValueMap<ValType>::operator=(const MapType& val)
{
    setValue(val);
    return *this;
}

template <Type ValType>
void ValueMap<ValType>::setValue(const MapType& val)
{
    if (m_value == val)
        return;

    m_parent->valueUpdated(*this);
    m_value = val;
}

template <Type ValType>
bool ValueMap<ValType>::contains(const std::string& key) const
{
    return std::find_if(m_value.begin(), m_value.end(), [&](const auto& pair) {
        return pair.first == key;
    }) != m_value.end();
}

template <Type ValType>
bool ValueMap<ValType>::compare(const Attribute& other) const
{
    if (auto casted = dynamic_cast<const ValueMap<ValType>*>(&other)) {
        return casted->value() == value();
    }
    return false;
}

template <Type ValType>
std::string ValueMap<ValType>::typeName() const
{
    return "Map";
}

template <Type ValType>
void ValueMap<ValType>::set(const Attribute& other)
{
    if (auto casted = dynamic_cast<const ValueMap<ValType>*>(&other)) {
        setValue(casted->value());
    }
}

template <Type ValType>
void ValueMap<ValType>::set(Attribute&& other)
{
    if (auto casted = dynamic_cast<ValueMap<ValType>*>(&other)) {
        setValue(std::move(casted->value()));
    }
}

template <Type ValType>
bool ValueMap<ValType>::hasValue() const
{
    return !m_value.empty();
}

template <Type ValType>
void ValueMap<ValType>::clear()
{
    m_value.clear();
}

template <Type ValType>
Type ValueMap<ValType>::valueType() const
{
    return ThisType;
}

template <Type ValType>
void ValueMap<ValType>::append(const std::string& key, const CppType& val)
{
    m_value.emplace(key, val);
}

// ===========================================================================================================

} // namespace pack
