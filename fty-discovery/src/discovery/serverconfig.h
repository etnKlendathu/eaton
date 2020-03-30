#pragma once
#include "config.h"

namespace fty {

class ServerConfig: public Config
{
public:
    static ServerConfig& instance();
public:
    void load(const std::string& file);
private:
    ServerConfig() = default;
};

}
