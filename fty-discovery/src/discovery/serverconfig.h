#pragma once
#include "config.h"

namespace fty {

class ServerConfig : public Config
{
public:
    static ServerConfig& instance();

public:
    void               load(const std::string& file);
    const std::string& fileName() const;

private:
    ServerConfig() = default;
    std::string m_fileName;
};

} // namespace fty
