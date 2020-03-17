#include "classgenerator.h"
#include "formatter.h"
#include <fty/split.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/extension_set.h>
#include <iostream>

namespace google::protobuf::compiler::fty {

ClassGenerator::ClassGenerator(const Descriptor* desc)
    : m_desc(desc)
{
}

const Descriptor* ClassGenerator::descriptor() const
{
    return m_desc;
}

static std::string usingType(const protobuf::FieldDescriptor* fld)
{
    switch (fld->cpp_type()) {
        case FieldDescriptor::CPPTYPE_STRING:
            return "std::string";
        case FieldDescriptor::CPPTYPE_BOOL:
            return "bool";
        case FieldDescriptor::CPPTYPE_INT64:
            return "int64_t";
        case FieldDescriptor::CPPTYPE_INT32:
            return "int32_t";
        case FieldDescriptor::CPPTYPE_FLOAT:
            return "float";
        case FieldDescriptor::CPPTYPE_DOUBLE:
            return "double";
        case FieldDescriptor::CPPTYPE_UINT32:
            return "uint32_t";
        case FieldDescriptor::CPPTYPE_UINT64:
            return "uint64_t";
        case FieldDescriptor::CPPTYPE_ENUM:
            return fld->enum_type()->name();
        case FieldDescriptor::CPPTYPE_MESSAGE: {
            std::string            name = fld->message_type()->full_name();
            std::string::size_type n    = 0;
            while ((n = name.find(".", n)) != std::string::npos) {
                name.replace(n, 1, "::");
                ++n;
            }
            return name;
        }
    }
    return {};
}

static std::vector<const FieldDescriptor*> getExtensions(const FileDescriptor* file)
{
    std::vector<const FieldDescriptor*> ret;
    // std::cerr << file->name() << file->extension_count() << std::endl;
    for (int j = 0; j < file->dependency_count(); ++j) {
        auto dep = file->dependency(j);
        for (int i = 0; i < dep->extension_count(); ++i) {
            if (dep->extension(i)->name().find("DefaultValue") != std::string::npos) {
                ret.push_back(dep->extension(i));
            }
        }
    }

    for (int i = 0; i < file->extension_count(); ++i) {
        if (file->extension(i)->name().find("DefaultValue") != std::string::npos) {
            ret.push_back(file->extension(i));
        }
    }
    return ret;
}

void ClassGenerator::generateHeader(Formatter& frm, const std::string& descNamespace, bool asMap) const
{
    frm << "class " << m_desc->name() << ": public pack::Node"
        << "\n";
    frm << "{\n";
    frm << "public:\n";
    frm.indent();
    frm << "using pack::Node::Node;\n\n";
    if (asMap) {
        std::string key;
        std::string value;

        for (int i = 0; i < m_desc->field_count(); ++i) {
            const auto& fld = m_desc->field(i);
            if (fld->name() == "key") {
                key = usingType(fld);
            }
            if (fld->name() == "value") {
                value = usingType(fld);
            }
        }

        frm << "using KeyType = " << key << ";\n";
        frm << "using ValueType = " << value << ";\n";
    }
    frm.outdent();

    if (m_desc->nested_type_count()) {
        frm << "public:\n";
        frm.indent();
        for (int i = 0; i < m_desc->nested_type_count(); ++i) {
            const auto& type = m_desc->nested_type(i);

            bool usedInMap = false;
            for (int i = 0; i < m_desc->field_count(); ++i) {
                const auto& fld = m_desc->field(i);
                if (fld->is_map() && fld->message_type() == type) {
                    usedInMap = true;
                    break;
                }
            }

            ClassGenerator nested(type);
            nested.generateHeader(frm, descNamespace, usedInMap);
        }
    }

    if (m_desc->enum_type_count()) {
        frm << "public:\n";
        frm.indent();
        for (int i = 0; i < m_desc->enum_type_count(); ++i) {
            const auto& type = m_desc->enum_type(i);
            frm << "enum class " << type->name() << "\n";
            frm << "{\n";
            frm.indent();
            for (int j = 0; j < type->value_count(); ++j) {
                const auto& val = type->value(j);
                frm << val->name() << " = " << val->number() << ",\n";
            }
            frm.outdent();
            frm << "};\n";
        }
    }

    frm << "public:\n";
    frm.indent();

    auto deps = getExtensions(m_desc->file());

    for (int i = 0; i < m_desc->field_count(); ++i) {
        const auto& fld = m_desc->field(i);

        std::string         def;
        const FieldOptions& opts = fld->options();
        for (auto dep : deps) {
            auto [id, val] = ::fty::split<int, std::string>(opts.DebugString(), ":");
            if (dep->number() == id) {
                def = val;
            }
        }

        if (!def.empty()) {
            def = ", " + def;
        }

        frm << cppType(fld) << " " << fld->camelcase_name() << " = FIELD(\"" << fld->name() << "\"" << def
            << ")"
            << ";\n";
    }

    frm << "\n";
    frm << "META(" << m_desc->name();
    for (int i = 0; i < m_desc->field_count(); ++i) {
        const auto& fld = m_desc->field(i);
        frm << ", " << fld->camelcase_name();
    }
    frm << ")\n";

    frm.outdent();
    frm << "public:\n";
    frm.indent();

    if (m_desc->oneof_decl_count()) {
        frm << "void valueUpdated(const Attribute& attr) override\n";
        frm << "{\n";
        frm.indent();
        frm << "if (!m_silent) {\n";
        frm.indent();
        frm << "m_silent = true;\n";
        for (int i = 0; i < m_desc->oneof_decl_count(); ++i) {
            const auto& oneof = m_desc->oneof_decl(i);
            frm << "if (auto it = std::find(m_" << oneof->name() << ".begin(), m_" << oneof->name()
                << ".end(), &attr); it != m_" << oneof->name() << ".end()) {\n";
            frm.indent();
            frm << "for(auto& val: m_" << oneof->name() << ") {\n";
            frm.indent();
            frm << "if (val != *it) {\n";
            frm.indent();
            frm << "val->clear();\n";
            frm.outdent();
            frm << "}\n";
            frm.outdent();
            frm << "}\n";
            frm.outdent();
            frm << "}\n";
            frm << "m_silent = false;\n";
            frm.outdent();
            frm << "}\n";
        }
        frm << "if (m_parent) {\n";
        frm.indent();
        frm << "m_parent->valueUpdated(*this);\n";
        frm.outdent();
        frm << "}\n";
        frm.outdent();

        frm << "}\n\n";
    }

    frm << "const std::string& fileDescriptor() const override\n";
    frm << "{\n";
    frm.indent();
    frm << "return " << descNamespace << "::descriptor();\n";
    frm.outdent();
    frm << "}\n\n";

    frm << "std::string protoName() const override\n";
    frm << "{\n";
    frm.indent();
    frm << "return \"" << m_desc->full_name() << "\";\n";
    frm.outdent();
    frm << "}\n\n";
    frm.outdent();

    if (m_desc->oneof_decl_count()) {
        frm << "private:\n";
        frm.indent();
        for (int i = 0; i < m_desc->oneof_decl_count(); ++i) {
            const auto& oneof = m_desc->oneof_decl(i);
            frm << "std::vector<Attribute*> m_" << oneof->name() << " = {";
            bool first = true;
            for (int i = 0; i < oneof->field_count(); ++i) {
                frm << (!first ? ", " : "") << "&" << oneof->field(i)->name();
                first = false;
            }
            frm << "};\n";
        }
        frm << "bool m_silent = false;\n";
    }

    frm.outdent();
    frm << "};\n";
    frm << "\n";
}

std::string ClassGenerator::cppType(const FieldDescriptor* fld) const
{
    using namespace std::string_literals;
    bool isList = fld->is_repeated();
    switch (fld->cpp_type()) {
        case FieldDescriptor::CPPTYPE_STRING:
            return "pack::String"s + (isList ? "List" : "");
        case FieldDescriptor::CPPTYPE_BOOL:
            return "pack::Bool"s + (isList ? "List>" : "");
        case FieldDescriptor::CPPTYPE_INT64:
            return "pack::Int64"s + (isList ? "List" : "");
        case FieldDescriptor::CPPTYPE_INT32:
            return "pack::Int32"s + (isList ? "List" : "");
        case FieldDescriptor::CPPTYPE_FLOAT:
            return "pack::Float"s + (isList ? "List" : "");
        case FieldDescriptor::CPPTYPE_DOUBLE:
            return "pack::Double"s + (isList ? "List" : "");
        case FieldDescriptor::CPPTYPE_UINT32:
            return "pack::UInt32"s + (isList ? "List" : "");
        case FieldDescriptor::CPPTYPE_UINT64:
            return "pack::UInt64"s + (isList ? "List" : "");
        case FieldDescriptor::CPPTYPE_ENUM:
            return "pack::Enum<" + std::string(fld->enum_type()->name()) + ">";
        case FieldDescriptor::CPPTYPE_MESSAGE: {
            std::string            name = fld->message_type()->full_name();
            std::string::size_type n    = 0;
            while ((n = name.find(".", n)) != std::string::npos) {
                name.replace(n, 1, "::");
                ++n;
            }
            if (fld->is_map()) {
                return "pack::ProtoMap<" + name + ">";
            } else {
                return isList ? "pack::ObjectList<" + name + ">" : name;
            }
        }
    }
}

} // namespace google::protobuf::compiler::fty
