#include "filegenerator.h"
#include "classgenerator.h"
#include "formatter.h"
#include "utils.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <sstream>
#include <iomanip>

namespace google::protobuf::compiler::fty {

static void Flatten(const Descriptor* descriptor, std::vector<const Descriptor*>* flatten)
{
    for (int i = 0; i < descriptor->nested_type_count(); i++) {
        Flatten(descriptor->nested_type(i), flatten);
    }
    flatten->push_back(descriptor);
}

static std::vector<const Descriptor*> FlattenMessagesInFile(const FileDescriptor* file)
{
    std::vector<const Descriptor*> result;
    for (int i = 0; i < file->message_type_count(); i++) {
        Flatten(file->message_type(i), &result);
    }
    return result;
}

FileGenerator::FileGenerator(const FileDescriptor* file)
    : m_file(file)
{
    std::vector<const Descriptor*> msgs = FlattenMessagesInFile(m_file);
    for (const auto& msg : msgs) {
        m_generators.emplace_back(msg);
    }
}

void FileGenerator::generateHeader(io::Printer& printer) const
{
    Formatter frm(printer);

    frm << "#pragma once\n";

    for (int i = 0; i < m_file->dependency_count(); ++i) {
        auto dep = m_file->dependency(i);
        frm << "#include \"" << genFileName(dep) << "\"\n";
    }
    frm << "#include <pack/pack.h>\n";
    frm << "\n";

    std::string path  = "file";
    size_t      index = m_file->name().find_last_of('.');
    if (index != std::string::npos) {
        path                     = m_file->name().substr(0, index);
        std::string::size_type n = 0;
        while ((n = path.find("/", n)) != std::string::npos) {
            path.replace(n, 1, "::");
            ++n;
        }
        n = 0;
        while ((n = path.find("-", n)) != std::string::npos) {
            path.replace(n, 1, "_");
            ++n;
        }
    }

    std::stringstream ss;
    bool wasText = false;
    bool wasHex = false;
    for(const char& ch: getDescriptor()) {
        if (std::isalnum(ch)) {
            ss << (wasHex ? "\" \"" : "") << ch;
            wasText = true;
            wasHex = false;
        } else {
            ss << (wasText ? "\" \"" : "") << "\\x" <<  std::setfill('0') << std::setw(2) << std::hex << static_cast<uint32_t>(static_cast<uint8_t>(ch));
            wasHex = true;
            wasText = false;
        }
    }
    frm << "// "
           "================================================================================================="
           "==========\n";
    frm << "// Protobuf file descriptor\n";
    frm << "// "
           "================================================================================================="
           "==========\n";
    frm << "namespace " << path << " {\n\n";
    frm << "inline const std::string& descriptor()\n";
    frm << "{\n";
    frm.indent();
    frm << "static std::string desc(\"" << ss.str() << "\", " << ss.str().size() << ");\n";
    frm << "return desc;\n";
    frm.outdent();
    frm << "}\n";
    frm << "}\n\n";

    frm << "// "
           "================================================================================================="
           "==========\n";
    frm << "// Pack files\n";
    frm << "// "
           "================================================================================================="
           "==========\n";
    if (!m_file->package().empty()) {
        frm << "namespace " << m_file->package() << " {\n\n";
    }

    for (int i = 0; i < m_file->message_type_count(); i++) {
        ClassGenerator gen(m_file->message_type(i));
        gen.generateHeader(frm, path);
    }

    if (!m_file->package().empty()) {
        frm << "}\n";
    }
}

static void grabDependencies(std::set<const FileDescriptor*>& toWrite, const FileDescriptor* desc)
{
    for (int i = 0; i < desc->dependency_count(); i++) {
        const FileDescriptor* dependency = desc->dependency(i);
        toWrite.insert(dependency);
        grabDependencies(toWrite, dependency);
    }
}

std::string FileGenerator::getDescriptor() const
{
    FileDescriptorSet               fileSet;
    std::set<const FileDescriptor*> toWrite;
    toWrite.insert(m_file);
    grabDependencies(toWrite, m_file);

    auto file = fileSet.mutable_file();
    for (const auto& desc : toWrite) {
        auto newDep = file->Add();
        desc->CopyTo(newDep);
    }

    return fileSet.SerializeAsString();
}

FileGenerator::~FileGenerator()
{
}

} // namespace google::protobuf::compiler::fty
