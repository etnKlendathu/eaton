#pragma once
#include "actor.h"
#include <fty/expected.h>

class Poller
{
public:
    template<typename... T>
    Poller(const T&... pipes):
        Poller(std::vector<IPipe*>{pipes...})
    {}

    Poller(std::vector<IPipe*> pipes);

    fty::Expected<IPipe*> wait(int timeout);
    void add(IPipe* pipes);
    void remove(IPipe* pipes);
private:
    zpoller_t* m_poller;
    std::map<void*, IPipe*> m_mapping;
};

