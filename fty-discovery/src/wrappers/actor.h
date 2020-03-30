#pragma once
#include <memory>
#include <czmq.h>
#include "zmessage.h"
#include "src/commands.h"

typedef struct _zsock_t zsock_t;

class IPipe
{
public:
    enum class Read
    {
        Wait,
        NoWait
    };

public:
    virtual ~IPipe() = default;
    virtual zsock_t* pipe() = 0;
    virtual ZMessage read(Read readOp = Read::Wait) const = 0;
};

template<typename T>
class Actor: public IPipe
{
public:
    virtual ~Actor()
    {
        zactor_destroy(&m_actor);
    }

    template<typename... Args>
    bool run(Args&&... params)
    {
        m_caller = [&](){
            std::invoke(&T::runWorker, static_cast<T*>(this), std::forward<Args>(params)...);
//            std::apply([&](auto&&... args) {
//                static_cast<T*>(this)->runWorker(args...);
//            }, std::move(args));
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
        send(std::move(ZMessage::create(cmd...)));
    }

    ZMessage read(Read readOp = Read::Wait) const override
    {
        ZMessage msg(readOp == Read::Wait ? zmsg_recv(m_socket) : zmsg_recv_nowait(m_socket));
        return msg;
    }

    void send(ZMessage&& message)
    {
        zmsg_t* msg = message.take();
        zmsg_send(&msg, m_socket);
    }
protected:
    bool informAndWait()
    {
        bool stopNow = true;

        write(discovery::Command::InfoReady);

        if (ZMessage msgRun = read()) {
            auto cmd = *msgRun.pop<discovery::Command>();
            if (cmd == discovery::Command::Continue) {
                stopNow = false;
            }
        }

        if (zsys_interrupted) {
            stopNow = true;
        }

        return stopNow;
    }

    bool askActorTerm()
    {
        if (ZMessage msgStop = read(Read::NoWait)) {
            auto cmd = *msgStop.pop<discovery::Command>();
            if (cmd == discovery::Command::Term) {
                return true;
            }
        }
        return false;
    }

private:
    static void worker(zsock_t* sock, void* args)
    {
        auto self = reinterpret_cast<Actor<T>*>(args);
        self->m_socket = sock;
        zsock_signal(self->m_socket, 0);
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

