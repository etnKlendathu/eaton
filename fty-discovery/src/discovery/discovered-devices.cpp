#include "discovered-devices.h"
#include <algorithm>

namespace fty {

bool DiscoveredDevices::containsIp(const std::string& ip) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto                        it = std::find_if(m_list.begin(), m_list.end(), [&](const auto& pair) {
        return pair.second == ip;
    });
    return it != m_list.end();
}

void DiscoveredDevices::remove(const std::string& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_list.erase(key);
}

void DiscoveredDevices::emplace(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_list.emplace(key, value);
}

std::mutex& DiscoveredDevices::mutex() const
{
    return m_mutex;
}

DiscoveredDevices::Iterator DiscoveredDevices::begin()
{
    return m_list.begin();
}

DiscoveredDevices::Iterator DiscoveredDevices::end()
{
    return m_list.end();
}

DiscoveredDevices::ConstIterator DiscoveredDevices::begin() const
{
    return m_list.begin();
}

DiscoveredDevices::ConstIterator DiscoveredDevices::end() const
{
    return m_list.end();
}


}
