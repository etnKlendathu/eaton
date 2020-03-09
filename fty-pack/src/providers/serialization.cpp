#include <fstream>
#include "pack/serialization.h"

Expected<std::string> read(const std::string& filename)
{
    std::ifstream st(filename);
    if (st.is_open()) {
        return Expected<std::string>({std::istreambuf_iterator<char>(st), std::istreambuf_iterator<char>()});
    }
    return unexpected() << "Cannot read file" << filename;
}
