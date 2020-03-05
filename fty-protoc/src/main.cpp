#include <google/protobuf/compiler/plugin.h>
#include "generator.h"

int main(int argc, char* argv[])
{
    google::protobuf::compiler::fty::Generator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
