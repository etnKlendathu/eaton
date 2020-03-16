#pragma once
#include <google/protobuf/descriptor.h>

namespace google::protobuf::compiler::fty {

class Formatter;
class ClassGenerator
{
public:
    ClassGenerator(const Descriptor* desc);

    const Descriptor* descriptor() const;

    void generateHeader(Formatter& printer, const std::string& descNamespace, bool asMap = false) const;
private:
    std::string cppType(const FieldDescriptor* fld) const;
private:
    const Descriptor* m_desc;
};

}
