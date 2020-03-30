#include <fstream>
#include "pack/serialization.h"

namespace pack {

fty::Expected<std::string> read(const std::string& filename)
{
    std::ifstream st(filename);
    if (st.is_open()) {
        return fty::Expected<std::string>({std::istreambuf_iterator<char>(st), std::istreambuf_iterator<char>()});
    }
    return fty::unexpected() << "Cannot read file" << filename;
}

}
