#include "ftyproto.h"

FtyProto::FtyProto()
{
}

FtyProto::FtyProto(int id)
    : m_proto(fty_proto_new(id))
{
}

FtyProto::FtyProto(fty_proto_t* msg)
    : m_proto(msg)
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

ZMessage FtyProto::encode() const
{
    fty_proto_t* dup = fty_proto_dup(m_proto);
    return {fty_proto_encode(&dup)};
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

void FtyProto::extInsert(const std::string& key, const std::string& val)
{
    fty_proto_ext_insert(m_proto, key.c_str(), "%s", val.c_str());
}

std::map<std::string, std::string> FtyProto::aux() const
{
    std::map<std::string, std::string> map;
    zhash_t*                           aux = fty_proto_aux(m_proto);
    for (auto* it = zhash_first(aux); it; it = zhash_next(aux)) {
        map.emplace(zhash_cursor(aux), static_cast<const char*>(it));
    }
    return map;
}

std::string FtyProto::auxString(const std::string& key, const std::string& def) const
{
    if (auto it = fty_proto_aux_string(m_proto, key.c_str(), nullptr)) {
        return it;
    }
    return def;
}

void FtyProto::setOperation(const std::string& operation)
{
    fty_proto_set_operation(m_proto, operation.c_str());
}

void FtyProto::print() const
{
    fty_proto_print(m_proto);
}
