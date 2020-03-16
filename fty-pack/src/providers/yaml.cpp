#include "pack/pack.h"
#include "pack/serialization.h"
#include "visitor.h"
#include <iostream>
#include <yaml-cpp/yaml.h>

namespace pack {

// ===========================================================================================================

template <Type ValType>
struct Convert
{
    using CppType = typename ResolveType<ValType>::type;

    static void decode(Value<ValType>& node, const YAML::Node& yaml)
    {
        node = yaml.as<CppType>();
    }

    static void decode(ValueList<ValType>& node, const YAML::Node& yaml)
    {
        for (const auto& it : yaml) {
            node.append(it.as<CppType>());
        }
    }

    static void decode(ValueMap<ValType>& node, const YAML::Node& yaml)
    {
        for (const auto& it : yaml) {
            node.append(it.first.as<std::string>(), it.second.as<CppType>());
        }
    }

    static void encode(const Value<ValType>& node, YAML::Node& yaml)
    {
        yaml = YAML::convert<CppType>::encode(node.value());
    }

    static void encode(const ValueList<ValType>& node, YAML::Node& yaml)
    {
        if constexpr (ValType == Type::Bool) {
            for (auto it : node) {
                yaml.push_back(YAML::convert<CppType>::encode(it));
            }
        } else {
            for (const auto& it : node) {
                yaml.push_back(YAML::convert<CppType>::encode(it));
            }
        }
    }

    static void encode(const ValueMap<ValType>& node, YAML::Node& yaml)
    {
        for (const auto& it : node) {
            yaml[it.first] = YAML::convert<CppType>::encode(it.second);
        }
    }
};

// ===========================================================================================================

class YamlDeserializer : public Deserialize<YamlDeserializer>
{
public:
    template <typename T>
    static void unpackValue(T& val, const YAML::Node& yaml)
    {
        Convert<T::ThisType>::decode(val, yaml);
    }

    static void unpackValue(IEnum& en, const YAML::Node& yaml)
    {
        en.fromString(yaml.as<std::string>());
    }

    static void unpackValue(IObjectList& list, const YAML::Node& yaml)
    {
        for (const auto& child : yaml) {
            auto& obj = list.create();
            visit(obj, child);
        }
    }

    static void unpackValue(INode& node, const YAML::Node& yaml)
    {
        for (auto& it : node.fields()) {
            auto found = yaml[it->key()];
            if (found.IsDefined()) {
                visit(*it, found);
            }
        }
    }

    static void unpackValue(IProtoMap& map, const YAML::Node& yaml)
    {
        for (const auto& child : yaml) {
            INode& obj = map.create();

            YAML::Node temp;
            temp["key"] = child.first;
            temp["value"] = child.second;

            visit(obj, temp);
        }
    }

};

// ===========================================================================================================

class YamlSerializer : public Serialize<YamlSerializer>
{
public:
    template <typename T>
    static void packValue(const T& val, YAML::Node& yaml)
    {
        if (val.hasValue()) {
            Convert<T::ThisType>::encode(val, yaml);
        }
    }

    static void packValue(const IObjectList& val, YAML::Node& yaml)
    {
        for (int i = 0; i < val.size(); ++i) {
            const INode& node = val.get(i);
            YAML::Node   child;
            visit(node, child);
            yaml.push_back(child);
        }
    }

    static void packValue(const INode& node, YAML::Node& yaml)
    {
        for (auto& it : node.fields()) {
            if (node.hasValue()) {
                YAML::Node child = yaml[it->key()];
                visit(*it, child);
            }
        }
    }

    static void packValue(const IEnum& en, YAML::Node& yaml)
    {
        yaml = YAML::convert<std::string>::encode(en.asString());
    }

    static void packValue(const IProtoMap& map, YAML::Node& yaml)
    {
        for(int i = 0; i < map.size(); ++i) {
            const INode& node = map.get(i);

            YAML::Node temp;
            packValue(node, temp);

            yaml[temp["key"]] = temp["value"];
        }
    }
};

// ===========================================================================================================

namespace yaml {
    std::string serialize(const INode& node)
    {
        YAML::Node yaml;
        YamlSerializer::visit(node, yaml);
        return YAML::Dump(yaml);
    }

    void deserialize(const std::string& content, INode& node)
    {
        YAML::Node yaml = YAML::Load(content);
        YamlDeserializer::visit(node, yaml);
    }
} // namespace yaml

// ===========================================================================================================

namespace json {
    std::string serialize(const INode& node)
    {
        YAML::Node yaml;
        YamlSerializer::visit(node, yaml);

        YAML::Emitter out;
        out.SetIndent(4);

        out << YAML::DoubleQuoted << YAML::Flow << yaml;
        return out.c_str();
    }

    void deserialize(const std::string& content, INode& node)
    {
        YAML::Node yaml = YAML::Load(content);
        YamlDeserializer::visit(node, yaml);
    }
} // namespace json

// ===========================================================================================================

} // namespace pack
