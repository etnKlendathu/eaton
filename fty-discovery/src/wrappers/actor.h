#pragma once
#include <memory>
#include "zmessage.h"

typedef struct _zsock_t zsock_t;

class ActorImpl
{
public:
    ActorImpl();
    virtual ~ActorImpl();

    bool init();
    virtual void worker(zsock_t* sock) = 0;
    void send(ZMessage&& message);
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    friend class Actor;
};

class Actor
{
public:
    void write(ZMessage&& message);
    ZMessage read();

protected:
    Actor(std::unique_ptr<ActorImpl>&& impl);

protected:
    std::unique_ptr<ActorImpl> m_impl;
};
