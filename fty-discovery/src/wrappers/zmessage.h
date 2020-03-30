#pragma once
#include <czmq.h>
#include <fty/expected.h>
#include <fty_proto.h>
#include <memory>

class ZMessage
{
public:
    ZMessage(zmsg_t* msg)
        : m_message(msg, &ZMessage::freeMessage)
    {
    }

    ZMessage()
        : m_message(zmsg_new(), &ZMessage::freeMessage)
    {
    }

    ~ZMessage()
    {
    }

    ZMessage(const ZMessage&) = delete;
    ZMessage& operator=(const ZMessage&) = delete;
    ZMessage(ZMessage&&)                 = default;
    ZMessage& operator=(ZMessage&&) = default;

    template <typename T>
    fty::Expected<T> pop() const
    {
        if (char* str = zmsg_popstr(m_message.get())) {
            std::string ret = str;
            zstr_free(&str);
            return std::move(fty::convert<T>(ret));
        }
        return fty::unexpected("Empty command");
    }

    template <typename T>
    void add(const T& str)
    {
        zmsg_addstr(m_message.get(), fty::convert<std::string>(str).c_str());
    }

    template <typename T>
    void prepend(const T& str)
    {
        zmsg_pushstr(m_message.get(), fty::convert<std::string>(str).c_str());
    }

    void print() const
    {
        zmsg_print(m_message.get());
    }

    operator bool() const
    {
        return m_message.get() != nullptr;
    }

    zmsg_t* take()
    {
        return m_message.release();
    }

    bool isFtyProto() const
    {
        return is_fty_proto(m_message.get());
    }

    template <typename... Args>
    static ZMessage create(const Args&... args)
    {
        ZMessage msg;
        (msg.add(args), ...);
        return msg;
    }

private:
    static void freeMessage(zmsg_t* msg)
    {
        zmsg_destroy(&msg);
    }

private:
    std::unique_ptr<zmsg_t, decltype(&ZMessage::freeMessage)> m_message;
};
