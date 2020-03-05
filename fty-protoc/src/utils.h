#pragma once
#include <google/protobuf/descriptor.h>

namespace google::protobuf::compiler::fty {

inline std::string genFileName(const FileDescriptor* file)
{
    std::string name = file->name();
    size_t index = name.find_last_of('.');
    if (index != std::string::npos) {
        return name.substr(0, index) + ".h";
    }
    return name;
}

}
