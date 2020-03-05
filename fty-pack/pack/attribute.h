#pragma once

#include <optional>
#include <string>
#include <vector>

namespace pack {

// ===========================================================================================================

#define FIELD(key, id, ...)                                                                                  \
    {                                                                                                        \
        this, key, ##__VA_ARGS__                                                                             \
    }

#define META_CTR(className, ...)                                                                             \
    className(const className& other)                                                                        \
    {                                                                                                        \
        for (auto& it : fields()) {                                                                          \
            it->set(*other.fieldByKey(it->key()));                                                           \
        }                                                                                                    \
    }                                                                                                        \
    className(className&& other)                                                                             \
    {                                                                                                        \
        for (auto& it : fields()) {                                                                          \
            it->set(std::move(*other.fieldByKey(it->key())));                                                \
        }                                                                                                    \
    }                                                                                                        \
    className& operator=(const className& other)                                                             \
    {                                                                                                        \
        for (auto& it : fields()) {                                                                          \
            it->set(*other.fieldByKey(it->key()));                                                           \
        }                                                                                                    \
        return *this;                                                                                        \
    }                                                                                                        \
    className& operator=(className&& other)                                                                  \
    {                                                                                                        \
        for (auto& it : fields()) {                                                                          \
            it->set(std::move(*other.fieldByKey(it->key())));                                                \
        }                                                                                                    \
        return *this;                                                                                        \
    }

#define META_FIELDS(className, ...)                                                                          \
public:                                                                                                      \
    std::vector<const pack::Attribute*> fields() const override                                              \
    {                                                                                                        \
        return std::apply(                                                                                   \
            [](const auto&... elems) {                                                                       \
                return std::vector<const pack::Attribute*>{&elems...};                                       \
            },                                                                                               \
            std::forward_as_tuple(__VA_ARGS__));                                                             \
    }                                                                                                        \
    std::vector<pack::Attribute*> fields() override                                                          \
    {                                                                                                        \
        return std::apply(                                                                                   \
            [](auto&... elems) {                                                                             \
                return std::vector<pack::Attribute*>{&elems...};                                             \
            },                                                                                               \
            std::forward_as_tuple(__VA_ARGS__));                                                             \
    }                                                                                                        \
    std::vector<std::string> fieldNames() const override                                                     \
    {                                                                                                        \
        return pack::split(#__VA_ARGS__);                                                                    \
    }                                                                                                        \
    std::string typeName() const override                                                                    \
    {                                                                                                        \
        return #className;                                                                                   \
    }                                                                                                        \
    static std::string typeInfo()                                                                            \
    {                                                                                                        \
        return #className;                                                                                   \
    }

#define META(className, ...)                                                                                 \
public:                                                                                                      \
    META_FIELDS(className, __VA_ARGS__)                                                                      \
    META_CTR(className, __VA_ARGS__)

#define META_WOC(className, ...)                                                                             \
public:                                                                                                      \
    META_FIELDS(className, __VA_ARGS__)

// ===========================================================================================================

std::vector<std::string> split(const std::string& str);

// ===========================================================================================================

class Attribute
{
public:
    enum class NodeType
    {
        Node,
        Value,
        Enum,
        List,
        Map
    };

public:
    // Attribute();
    Attribute(NodeType type, Attribute* parent, const std::string& key = {});
    virtual ~Attribute() = default;

    virtual bool        compare(const Attribute& other) const = 0;
    virtual void        valueUpdated(const Attribute& attr);
    virtual std::string typeName() const            = 0;
    virtual void        set(const Attribute& other) = 0;
    virtual void        set(Attribute&& other)      = 0;
    virtual bool        hasValue() const            = 0;
    virtual void        clear()                     = 0;

    const std::string& key() const;

    bool operator==(const Attribute& other) const;
    bool operator!=(const Attribute& other) const;

    const Attribute* parent() const;
    NodeType         type() const;

protected:
    Attribute*  m_parent = nullptr;
    std::string m_key;
    NodeType    m_type;
};

// ===========================================================================================================

} // namespace pack
