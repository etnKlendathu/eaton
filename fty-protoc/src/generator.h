#pragma once
#include <google/protobuf/compiler/code_generator.h>

namespace google::protobuf::compiler::fty {

class Generator : public google::protobuf::compiler::CodeGenerator
{
public:
    Generator();

    bool Generate(const FileDescriptor* file, const std::string& parameter,
        GeneratorContext* generator_context, std::string* error) const override;
};

} // namespace google::protobuf::compiler::fty
