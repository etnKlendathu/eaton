#pragma once
#include "actor.h"
#include <fty/expected.h>
#include <memory>

struct _zmsg_t;
using zmsg_t = _zmsg_t;

struct _zsock_t;
using zsock_t = _zsock_t;

class ZMessage;

class Mlm : public IPipe
{
public:
    Mlm();
    ~Mlm() override;

    bool               connect(const std::string& endpoint, uint32_t timeout, const std::string& address);
    fty::Expected<int> sendto(const std::string& address, const std::string& subject,
        const std::string& tracker, uint32_t timeout, zmsg_t** content);
    fty::Expected<int> sendto(const std::string& address, const std::string& subject,
        const std::string& tracker, uint32_t timeout, ZMessage&& content);
    fty::Expected<int> setConsumer(const std::string& stream, const std::string& pattern);
    std::string        sender();
    std::string        subject();
    std::string        tracker();
    std::string        command();

public:
    ZMessage read(Read readOp = Read::Wait) const override;
    zsock_t* pipe() override;


    // mlm_client_msgpipe
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
