#include "mlm.h"
#include <malamute.h>
#include <mlm_client.h>
#include "zmessage.h"

// ===========================================================================================================

class Mlm::Impl
{
public:
    Impl()
        : m_client(mlm_client_new(), &Impl::destroyclient)
    {
    }

    static void destroyclient(mlm_client_t* client)
    {
        mlm_client_destroy(&client);
    }

private:
    std::unique_ptr<mlm_client_t, decltype(&Impl::destroyclient)> m_client;
    friend class Mlm;
};

// ===========================================================================================================

Mlm::Mlm()
    : m_impl(new Impl)
{
}

Mlm::~Mlm()
{
}

bool Mlm::connect(const std::string& endpoint, uint32_t timeout, const std::string& address)
{
    return mlm_client_connect(m_impl->m_client.get(), endpoint.c_str(), timeout, address.c_str()) >= 0;
}

fty::Expected<int> Mlm::sendto(const std::string& address, const std::string& subject, const std::string& tracker,
    uint32_t timeout, zmsg_t** content)
{
    int res = mlm_client_sendto(
        m_impl->m_client.get(), address.c_str(), subject.c_str(), tracker.c_str(), timeout, content);
    if (res != -1) {
        return res;
    }
    return fty::unexpected() << "Failed to send " << subject << " message to asset-agent";
}

fty::Expected<int> Mlm::sendto(const std::string& address, const std::string& subject, const std::string& tracker,
    uint32_t timeout, ZMessage&& content)
{
    zmsg_t* msg = content.take();
    int res = mlm_client_sendto(
        m_impl->m_client.get(), address.c_str(), subject.c_str(), tracker.c_str(), timeout, &msg);
    if (res != -1) {
        return res;
    }
    return fty::unexpected() << "Failed to send " << subject << " message to asset-agent";
}


ZMessage Mlm::read(Read /*readOp*/) const
{
    return ZMessage(mlm_client_recv(m_impl->m_client.get()));
}

fty::Expected<int> Mlm::setConsumer(const std::string& stream, const std::string& pattern)
{
    int ret = mlm_client_set_consumer(m_impl->m_client.get(), stream.c_str(), pattern.c_str());
    if (ret == -1) {
        return fty::unexpected("malamut: cannot set consumer");
    }
    return ret;
}

std::string Mlm::sender()
{
    return mlm_client_sender(m_impl->m_client.get());
}

std::string Mlm::subject()
{
    return mlm_client_subject(m_impl->m_client.get());
}

std::string Mlm::tracker()
{
    return mlm_client_tracker(m_impl->m_client.get());
}

zsock_t* Mlm::pipe()
{
    return mlm_client_msgpipe(m_impl->m_client.get());
}

std::string Mlm::command()
{
    return mlm_client_command(m_impl->m_client.get());
}

// ===========================================================================================================
