#include "ftyproto.h"

FtyProto::FtyProto()
{}

FtyProto::FtyProto(int id):
    m_proto(fty_proto_new(id))
{
}

FtyProto::FtyProto(fty_proto_t* msg):
    m_proto(msg)
{
}

FtyProto::~FtyProto()
{
    fty_proto_destroy(&m_proto);
}

FtyProto::operator bool() const
{
    return m_proto != nullptr;
}

FtyProto FtyProto::decode(ZMessage&& msg)
{
    zmsg_t* temp = msg.take();
    return {fty_proto_decode(&temp)};
}

ZMessage FtyProto::encode()
{
    return {fty_proto_encode(&m_proto)};
}

int FtyProto::id() const
{
    return fty_proto_id(m_proto);
}

std::string FtyProto::name() const
{
    return fty_proto_name(m_proto);
}

std::string FtyProto::command() const
{
    return fty_proto_command(m_proto);
}

std::string FtyProto::operation() const
{
    return fty_proto_operation(m_proto);
}

std::string FtyProto::extString(const std::string& key) const
{
    return fty_proto_ext_string(m_proto, key.c_str(), nullptr);
}
