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
} // namespace zconfig

namespace protobuf {
    std::string serialize(const INode& node);
    void        deserialize(const std::string& content, INode& node);
}

} // namespace pack
