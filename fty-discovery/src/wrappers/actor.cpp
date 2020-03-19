#include "actor.h"
#include <czmq.h>
#include <sstream>
#include <random>
#include <thread>

class ActorImpl::Impl
{
public:
    Impl(ActorImpl* parent):
        m_parent(parent)
    {
    }

    ~Impl()
    {
        zactor_destroy(&m_actor);
    }

    bool init()
    {
        m_actor = zactor_new(&ActorImpl::Impl::worker, m_parent);
        return m_actor != nullptr;
    }

    static void worker(zsock_t* sock, void* args)
    {
        static_cast<ActorImpl*>(args)->worker(sock);
    }

    void send(ZMessage&& message)
    {
        zmsg_t* msg = message.take();
        zmsg_send(&msg, m_actor);
    }
private:
    ActorImpl* m_parent = nullptr;
    zactor_t* m_actor = nullptr;
    friend class ActorImpl;
};

ActorImpl::ActorImpl():
    m_impl(new Impl(this))
{
}

ActorImpl::~ActorImpl()
{
}

bool ActorImpl::init()
{
    return m_impl->init();
}

void ActorImpl::send(ZMessage&& message)
{
    m_impl->send(std::move(message));
}

void Actor::write(ZMessage&& message)
{
    m_impl->send(std::move(message));
}

ZMessage Actor::read()
{
    return {};
}

Actor::Actor(std::unique_ptr<ActorImpl>&& impl):
    m_impl(std::move(impl))
{}

