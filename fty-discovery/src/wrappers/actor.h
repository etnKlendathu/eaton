#pragma once
#include <memory>
#include <czmq.h>
#include "zmessage.h"

typedef struct _zsock_t zsock_t;

class IActor
{
public:
    virtual ~IActor() = default;
    virtual zsock_t* pipe() = 0;
    virtual ZMessage read() const = 0;
};

template<typename T>
class Actor: public IActor
{
public:
    virtual ~Actor()
    {
        zactor_destroy(&m_actor);
    }

    template<typename... Args>
    bool run(const Args&... params)
    {
        m_caller = [args = std::make_tuple(std::forward<Args>(params)...)](){
            std::apply([](auto&&... args) {
                T::run(args...);
            }, std::move(args));
        };
        m_actor = zactor_new(&Actor::worker, this);
        return m_actor != nullptr;
    }

    zsock_t* pipe() override
    {
        return m_socket;
    }

    template<typename... Args>
    void write(const Args&... cmd)
    {
        ZMessage msg;
        (msg.addStr(fty::convert<std::string>(cmd)),...);
        send(std::move(msg));
    }

    ZMessage read() const override
    {
        ZMessage msg(zmsg_recv(m_socket));
        return msg;
    }

    void send(ZMessage&& message)
    {
        zmsg_t* msg = message.take();
        zmsg_send(&msg, m_socket);
    }
private:
    static void worker(zsock_t* sock, void* args)
    {
        auto self = static_cast<Actor<T>>(args);
        self->m_socket = sock;
        self->m_caller();
    }
private:
    zactor_t* m_actor = nullptr;
    zsock_t* m_socket = nullptr;
    std::function<void()> m_caller;
};

class Finisher
{
public:
    Finisher(std::function<void()>&& caller):
        m_caller(caller)
    {}
    ~Finisher()
    {
        m_caller();
    }
private:
    std::function<void()> m_caller;
};

