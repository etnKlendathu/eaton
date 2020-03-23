#pragma once
#include "actor.h"
#include <fty/expected.h>

class Poller
{
public:
    template<typename... T>
    Poller(const T&... actors):
        Poller((actors)...)
    {}

    Poller(std::vector<IActor*> actors);

    fty::Expected<IActor*> wait(int timeout);
    void add(IActor* actor);
    void remove(IActor* actor);
private:
    zpoller_t* m_poller;
    std::map<void*, IActor*> m_mapping;
};

