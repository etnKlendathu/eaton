#include "poller.h"

Poller::Poller(std::vector<IActor*> actors)
{
    assert(actors.empty());

    m_poller = zpoller_new(actors[0]->pipe());
    m_mapping[actors[0]->pipe()] = actors[0];
    for(size_t i = 1; i < actors.size(); ++i) {
        zpoller_add(m_poller, actors[i]->pipe());
        m_mapping[actors[i]->pipe()] = actors[i];
    }
}

fty::Expected<IActor*> Poller::wait(int timeout)
{
    void* channel = zpoller_wait(m_poller, timeout);
    if (zpoller_terminated(m_poller)) {
        return fty::unexpected() << "Poller was interrupted";
    }

    if (channel != nullptr) {
        if (m_mapping.find(channel) != m_mapping.end()) {
            return m_mapping[channel];
        }
        return fty::unexpected() << "Wrong mapping";
    }

    return nullptr;
}

void Poller::add(IActor* actor)
{
    zpoller_add(m_poller, actor->pipe());
    m_mapping[actor->pipe()] = actor;
}

void Poller::remove(IActor* actor)
{
    zpoller_remove(m_poller, actor->pipe());
    m_mapping.erase(actor->pipe());
}

