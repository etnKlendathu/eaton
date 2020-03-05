#pragma once

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>

namespace google::protobuf::compiler::fty {

class ClassGenerator;

class FileGenerator
{
public:
    FileGenerator(const FileDescriptor* file);
    ~FileGenerator();

    void generateHeader(io::Printer& printer) const;
    std::string getDescriptor() const;
private:
    const FileDescriptor* m_file;
    std::vector<ClassGenerator> m_generators;
};

}
