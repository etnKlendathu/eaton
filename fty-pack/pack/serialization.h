#pragma once
#include <string>

namespace pack {

class INode;

namespace json {
    std::string serialize(const INode& node);
    void        deserialize(const std::string& content, INode& node);
} // namespace json

namespace yaml {
    std::string serialize(const INode& node);
    void        deserialize(const std::string& content, INode& node);
} // namespace yaml

namespace zconfig {
    std::string serialize(const INode& node);
    void        deserialize(const std::string& content, INode& node);

    template <typename T>
    T deserialize(const std::string& content)
    {
        T node;
        deserialize(content, node);
        return node;
    }
} // namespace zconfig

namespace protobuf {
    std::string serialize(const INode& node);
    void        deserialize(const std::string& content, INode& node);
}

} // namespace pack
