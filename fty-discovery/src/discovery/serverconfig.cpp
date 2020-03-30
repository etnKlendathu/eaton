#include "serverconfig.h"

namespace fty {

ServerConfig& ServerConfig::instance()
{
    static ServerConfig cfg;
    return cfg;
}

void ServerConfig::load(const std::string& file)
{
    clear();
    pack::zconfig::deserializeFile(file, *this);
     m_fileName = file;
}

const std::string& ServerConfig::fileName() const
{
    return m_fileName;
}

}
