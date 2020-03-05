#pragma once
#include "pack/attribute.h"
#include "pack/magic_enum.h"
#include <sstream>

namespace pack {

// ===========================================================================================================

/// Enum class interface
class IEnum : public Attribute
{
public:
    using ValuesType = std::vector<std::pair<std::string, int>>;

    IEnum(Attribute* parent, const std::string& key = {})
        : Attribute(NodeType::Enum, parent, key)
    {
    }

public:
    /// Returns a list of the enum values
    virtual ValuesType values() const = 0;

    /// Returns a string representation of the enum value
    virtual std::string asString() const = 0;

    /// Sets enum value from string
    virtual void fromString(const std::string& value) = 0;

    /// Returns a string representation of the enum value
    virtual int asInt() const = 0;

    /// Sets enum value from string
    virtual void fromInt(int value) = 0;
};

// ===========================================================================================================

template <typename T>
class Enum : public IEnum
{
public:
    using IEnum::IEnum;

public:
    const T& value() const;
             operator const T&() const;
    void     setValue(const T& val);
    void     setValue(T&& val);
    Enum&    operator=(const Enum& other);
    Enum&    operator=(Enum&& other);
    Enum&    operator=(const T& val);

private:
    static std::string asString(const T& value);

public:
    bool        compare(const Attribute& other) const override;
    std::string typeName() const override;
    ValuesType  values() const override;
    void        set(const Attribute& other) override;
    void        set(Attribute&& other) override;
    bool        hasValue() const override;
    std::string asString() const override;
    void        fromString(const std::string& value) override;
    int         asInt() const override;
    void        fromInt(int value) override;
    void        clear() override;

protected:
    T m_value = {};
    T m_def   = {};
};

// ===========================================================================================================
// Enum implementation
// ===========================================================================================================

template <typename T>
Enum<T>& Enum<T>::operator=(const Enum& other)
{
    setValue(other.m_value);
    return *this;
}

template <typename T>
Enum<T>& Enum<T>::operator=(Enum&& other)
{
    setValue(other.m_value);
    return *this;
}

template <typename T>
const T& Enum<T>::value() const
{
    return m_value;
}

template <typename T>
Enum<T>::operator const T&() const
{
    return m_value;
}

template <typename T>
Enum<T>& Enum<T>::operator=(const T& val)
{
    setValue(val);
    return *this;
}

template <typename T>
void Enum<T>::setValue(const T& val)
{
    if (m_value == val)
        return;

    m_parent->valueUpdated(*this);
    m_value = val;
}

template <typename T>
void Enum<T>::setValue(T&& val)
{
    if (m_value == val)
        return;

    m_parent->valueUpdated(*this);
    m_value = std::move(val);
}

template <typename T>
bool Enum<T>::compare(const Attribute& other) const
{
    if (auto casted = dynamic_cast<const Enum<T>*>(&other)) {
        return casted->value() == value();
    }
    return false;
}

template <typename T>
std::string Enum<T>::typeName() const
{
    return "Enum";
}

template <typename T>
typename Enum<T>::ValuesType Enum<T>::values() const
{
    std::vector<std::pair<std::string, int>> map;
    for (const auto& it : magic_enum::enum_values<T>()) {
        map.emplace_back(asString(it), int(it));
    }

    return map;
}

template <typename T>
std::string Enum<T>::asString(const T& value)
{
    using namespace magic_enum::ostream_operators;
    std::stringstream ss;
    ss << value;
    return ss.str();
}

template <typename T>
void Enum<T>::set(const Attribute& other)
{
    if (auto casted = dynamic_cast<const Enum<T>*>(&other)) {
        setValue(*casted);
    }
}

template <typename T>
void Enum<T>::set(Attribute&& other)
{
    if (auto casted = dynamic_cast<Enum<T>*>(&other)) {
        setValue(std::move(*casted));
    }
}

template <typename T>
bool Enum<T>::hasValue() const
{
    return m_value != m_def;
}

template <typename T>
std::string Enum<T>::asString() const
{
    return asString(value());
}

template <typename T>
void Enum<T>::fromString(const std::string& value)
{
    if (auto casted = magic_enum::enum_cast<T>(value)) {
        setValue(*casted);
    }
}

template <typename T>
int Enum<T>::asInt() const
{
    return int(value());
}

template <typename T>
void Enum<T>::fromInt(int value)
{
    if (auto casted = magic_enum::enum_cast<T>(value)) {
        setValue(*casted);
    }
}

template <typename T>
void Enum<T>::clear()
{
    setValue(m_def);
}

// ===========================================================================================================

} // namespace pack
