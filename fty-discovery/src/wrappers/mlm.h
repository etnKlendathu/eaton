#pragma once
#include <memory>
#include <utils/expected.h>

struct _zmsg_t;
using zmsg_t = _zmsg_t;

struct _zsock_t;
using zsock_t = _zsock_t;

class ZMessage;

class Mlm
{
public:
    Mlm();

    bool          connect(const std::string& endpoint, uint32_t timeout, const std::string& address);
    Expected<int> sendto(const std::string& address, const std::string& subject, const std::string& tracker,
        uint32_t timeout, zmsg_t** content);
    Expected<int> sendto(const std::string& address, const std::string& subject, const std::string& tracker,
        uint32_t timeout, ZMessage&& content);
    Expected<zmsg_t*> recv();
    Expected<int>     setConsumer(const std::string& stream, const std::string& pattern);
    std::string       sender();
    std::string       subject();
    std::string       tracker();
    zsock_t*          msgpipe();
    std::string       command();


    // mlm_client_msgpipe
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
