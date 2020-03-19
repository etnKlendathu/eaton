#pragma once
#include "zmessage.h"

typedef struct _fty_proto_t fty_proto_t;

class FtyProto
{
public:
    FtyProto();
    FtyProto(int id);
    FtyProto(fty_proto_t* msg);
    ~FtyProto();

    FtyProto(const FtyProto&) = delete;
    FtyProto& operator=(const FtyProto&) = delete;
    FtyProto(FtyProto&&) = default;
    FtyProto& operator=(FtyProto&&) = default;

    operator bool() const;
public:
    static FtyProto decode(ZMessage&& msg);
    ZMessage        encode();
    int             id() const;
    std::string     name() const;
    std::string     command() const;
    std::string     operation() const;
    std::string     extString(const std::string& key) const;

private:
    fty_proto_t* m_proto = nullptr;
};
