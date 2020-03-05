#include "classgenerator.h"
#include "formatter.h"

namespace google::protobuf::compiler::fty {

ClassGenerator::ClassGenerator(const Descriptor* desc)
    : m_desc(desc)
{
}

const Descriptor* ClassGenerator::descriptor() const
{
    return m_desc;
}

void ClassGenerator::generateHeader(Formatter& frm, const std::string& descNamespace) const
{
    frm << "class " << m_desc->name() << ": public pack::Node"
        << "\n";
    frm << "{\n";
    frm << "public:\n";
    frm.indent();
    frm << "using pack::Node::Node;\n\n";
    frm.outdent();

    if (m_desc->nested_type_count()) {
        frm << "public:\n";
        frm.indent();
        for (int i = 0; i < m_desc->nested_type_count(); ++i) {
            const auto& type = m_desc->nested_type(i);
            ClassGenerator nested(type);
            nested.generateHeader(frm, descNamespace);
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
            for(int j = 0; j < type->value_count(); ++j) {
                const auto& val = type->value(j);
                frm << val->name() << " = " << val->number() << ",\n";
            }
            frm.outdent();
            frm << "};\n";
        }
    }

    frm << "public:\n";
    frm.indent();

    for (int i = 0; i < m_desc->field_count(); ++i) {
        const auto& fld = m_desc->field(i);
        frm << cppType(fld) << " " << fld->camelcase_name() << " = FIELD(\"" << fld->name() << "\", "
            << fld->number() << ")"
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
        for(int i = 0; i < m_desc->oneof_decl_count(); ++i) {
            const auto& oneof = m_desc->oneof_decl(i);
            frm << "if (auto it = std::find(m_" << oneof->name() << ".begin(), m_" << oneof->name() << ".end(), &attr); it != m_" << oneof->name() << ".end()) {\n";
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
        for(int i = 0; i < m_desc->oneof_decl_count(); ++i) {
            const auto& oneof = m_desc->oneof_decl(i);
            frm << "std::vector<Attribute*> m_" << oneof->name() << " = {";
            bool first = true;
            for(int i = 0; i < oneof->field_count(); ++i) {
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
    bool isList = fld->is_repeated();
    switch (fld->cpp_type()) {
        case FieldDescriptor::CPPTYPE_STRING:
            return isList ? "pack::ValueList<pack::Type::String>" : "pack::String";
        case FieldDescriptor::CPPTYPE_BOOL:
            return isList ? "pack::ValueList<pack::Type::Bool>" : "pack::Bool";
        case FieldDescriptor::CPPTYPE_INT64:
            return isList ? "pack::ValueList<pack::Type::Int64>" : "pack::Int64";
        case FieldDescriptor::CPPTYPE_INT32:
            return isList ? "pack::ValueList<pack::Type::Int32>" : "pack::Int32";
        case FieldDescriptor::CPPTYPE_FLOAT:
            return isList ? "pack::ValueList<pack::Type::Float>" : "pack::Float";
        case FieldDescriptor::CPPTYPE_DOUBLE:
            return isList ? "pack::ValueList<pack::Type::Double" : "pack::Double";
        case FieldDescriptor::CPPTYPE_UINT32:
            return isList ? "pack::ValueList<pack::Type::UInt32>" : "pack::Uint32";
        case FieldDescriptor::CPPTYPE_UINT64:
            return isList ? "pack::ValueList<pack::Type::UInt64>" : "pack::Uint64";
        case FieldDescriptor::CPPTYPE_ENUM:
            return "pack::Enum<" + std::string(fld->enum_type()->name()) + ">";
        case FieldDescriptor::CPPTYPE_MESSAGE: {
            std::string name = fld->message_type()->full_name();
            std::string::size_type n = 0;
            while ((n = name.find(".", n)) != std::string::npos) {
                name.replace(n, 1, "::");
                ++n;
            }
            return isList ? "pack::ObjectList<" + name + ">" : name;
        }
    }
}

} // namespace google::protobuf::compiler::fty
