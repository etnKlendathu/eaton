#include "pack/pack.h"
#include <czmq.h>
#include <memory>
#include <zconfig.h>
#include "visitor.h"
#include <utils/convert.h>

namespace pack {

template <Type ValType>
struct Convert
{
    using CppType = typename ResolveType<ValType>::type;

    static void decode(Value<ValType>& node, zconfig_t* zconf)
    {
        node = fty::convert<CppType>(zconfig_value(zconf));
    }

    static void decode(ValueList<ValType>& node, zconfig_t* zconf)
    {
        for (zconfig_t* item = zconfig_child(zconf); item; item = zconfig_next(item)) {
            node.append(fty::convert<CppType>(zconfig_value(item)));
        }
    }

    static void decode(ValueMap<ValType>& node, zconfig_t* zconf)
    {
        for (zconfig_t* item = zconfig_child(zconf); item; item = zconfig_next(item)) {
            node.append(fty::convert<std::string>(zconfig_name(item)), fty::convert<CppType>(zconfig_value(item)));
        }
    }

    static void encode(const Value<ValType>& node, zconfig_t* zconf)
    {
        zconfig_set_value(zconf, "%s", fty::convert<std::string>(node.value()).c_str());
    }

    static void encode(const ValueList<ValType>& node, zconfig_t* zconf)
    {
        int i = 1;
        if constexpr (ValType == Type::Bool) {
            for (auto it : node) {
                auto child = zconfig_new(fty::convert<std::string>(i++).c_str(), zconf);
                zconfig_set_value(child, "%s", fty::convert<std::string>(it).c_str());
            }
        } else {
            for (const auto& it : node) {
                auto child = zconfig_new(fty::convert<std::string>(i++).c_str(), zconf);
                zconfig_set_value(child, "%s", fty::convert<std::string>(it).c_str());
            }
        }
    }

    static void encode(const ValueMap<ValType>& node, zconfig_t* zconf)
    {
        for (const auto& it : node) {
            auto child = zconfig_new(fty::convert<std::string>(it.first).c_str(), zconf);
            zconfig_set_value(child, "%s", fty::convert<std::string>(it.second).c_str());
        }
    }
};

class ZSerializer : public Serialize<ZSerializer>
{
public:
    template <typename T>
    static void packValue(const T& val, zconfig_t* zconf)
    {
        if (val.hasValue()) {
            Convert<T::ThisType>::encode(val, zconf);
        }
    }

    static void packValue(const IObjectList& val, zconfig_t* zconf)
    {
        for (int i = 0; i < val.size(); ++i) {
            const INode& node = val.get(i);
            auto child = zconfig_new(fty::convert<std::string>(i+1).c_str(), zconf);
            visit(node, child);
        }
    }

    static void packValue(const INode& node, zconfig_t* zconf)
    {
        for (auto& it : node.fields()) {
            if (it->hasValue()) {
                auto child = zconfig_new(it->key().c_str(), zconf);
                visit(*it, child);
            }
        }
    }

    static void packValue(const IEnum& en, zconfig_t* zconf)
    {
        zconfig_set_value(zconf, "%s", en.asString().c_str());
    }
};

class ZDeserializer : public Deserialize<ZDeserializer>
{
public:
    template <typename T>
    static void unpackValue(T& val, zconfig_t* conf)
    {
        Convert<T::ThisType>::decode(val, conf);
    }

    static void unpackValue(IEnum& en, zconfig_t* conf)
    {
        en.fromString(zconfig_value(conf));
    }

    static void unpackValue(IObjectList& list, zconfig_t* conf)
    {
        if (auto first = zconfig_child(conf)) {
            auto& obj = list.create();
            visit(obj, first);
            while((first = zconfig_next(first)) != nullptr) {
                auto& next = list.create();
                visit(next, first);
            }
        }
    }

    static void unpackValue(INode& node, zconfig_t* conf)
    {
        for (auto& it : node.fields()) {
            if (auto found = zconfig_locate(conf, it->key().c_str())) {
                visit(*it, found);
            }
        }
    }
};


namespace zconfig {
    std::string serialize(const INode& node)
    {
        zconfig_t* config = zconfig_new("root", nullptr);
        ZSerializer::visit(node, config);
        auto zret = zconfig_str_save(config);
        std::string ret(zret);
        zstr_free(&zret);
        zconfig_destroy(&config);
        return ret;
    }

    void deserialize(const std::string& content, INode& node)
    {
        zconfig_t* config = zconfig_str_load(content.c_str());
        ZDeserializer::visit(node, config);
        zconfig_destroy(&config);
    }
} // namespace zconfig

} // namespace pack
