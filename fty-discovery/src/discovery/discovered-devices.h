#pragma once
#include <map>
#include <mutex>

namespace fty {

// ===========================================================================================================

class DiscoveredDevices
{
public:
    using Container     = std::map<std::string, std::string>;
    using Iterator      = Container::iterator;
    using ConstIterator = Container::const_iterator;

public:
    DiscoveredDevices() = default;
    DiscoveredDevices(const DiscoveredDevices&) = delete;
    DiscoveredDevices& operator=(const DiscoveredDevices&) = delete;
    DiscoveredDevices(DiscoveredDevices&&);
    DiscoveredDevices& operator=(DiscoveredDevices&&);


    bool containsIp(const std::string& ip) const;
    void remove(const std::string& key);
    void emplace(const std::string& key, const std::string& value);
    bool empty() const;

    std::mutex& mutex() const;

    Iterator      begin();
    Iterator      end();
    ConstIterator begin() const;
    ConstIterator end() const;


private:
    mutable std::mutex m_mutex;
    Container          m_list;
};

// ===========================================================================================================

} // namespace fty
