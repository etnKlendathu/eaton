#include "poller.h"

Poller::Poller(std::vector<IPipe*> pipes)
{
    assert(!pipes.empty());

    m_poller = zpoller_new(pipes[0]->pipe());
    m_mapping[pipes[0]->pipe()] = pipes[0];
    for(size_t i = 1; i < pipes.size(); ++i) {
        zpoller_add(m_poller, pipes[i]->pipe());
        m_mapping[pipes[i]->pipe()] = pipes[i];
    }
}

fty::Expected<IPipe*> Poller::wait(int timeout)
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

void Poller::add(IPipe* pipes)
{
    zpoller_add(m_poller, pipes->pipe());
    m_mapping[pipes->pipe()] = pipes;
}

void Poller::remove(IPipe* pipes)
{
    zpoller_remove(m_poller, pipes->pipe());
    m_mapping.erase(pipes->pipe());
}

