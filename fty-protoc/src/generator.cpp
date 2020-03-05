#include "generator.h"
#include "filegenerator.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include "utils.h"
#include <cxxtools/posix/exec.h>

namespace google::protobuf::compiler::fty {

Generator::Generator()
{
}

bool Generator::Generate(const FileDescriptor* file, const std::string& /*parameter*/, GeneratorContext* context,
    std::string* /*error*/) const
{
    FileGenerator generator(file);

    {
        std::string fileName = genFileName(file);
        std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(fileName));
        io::Printer                               printer(output.get(), '$');
        generator.generateHeader(printer);

//        std::cerr << fileName << std::endl;
//        cxxtools::posix::Exec format("clang-format");
//        format.push_back(fileName);
//        format.push_back(" >> " + fileName);
//        format.exec();
    }

    return true;
}

} // namespace google::protobuf::compiler::fty
