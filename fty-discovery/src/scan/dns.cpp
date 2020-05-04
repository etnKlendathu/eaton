/*  =========================================================================
    scan_dns - collect information from DNS

    Copyright (C) 2014 - 2019 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/


#include "scan/dns.h"
#include "wrappers/ftyproto.h"
#include <fty/fty-log.h>
#include <sys/socket.h>

namespace fty::scan {

void Dns::scanDns(FtyProto& proto, const std::string& address)
{
    sockaddr_in saIn;
    sockaddr*   sa  = reinterpret_cast<sockaddr*>(&saIn);
    socklen_t   len = sizeof(sockaddr_in);

    char dnsName[NI_MAXHOST];

    saIn.sin_family = AF_INET;
    if (!inet_aton(address.c_str(), &saIn.sin_addr)) {
        return;
    }

    if (!getnameinfo(sa, len, dnsName, sizeof(dnsName), nullptr, 0, NI_NAMEREQD)) {
        proto.extInsert("dns.1", dnsName);
        logDbg() << "Retrieved DNS information";
        logDbg() << "FQDN =" << dnsName;

        char* p = strchr(dnsName, '.');
        if (p) {
            *p = 0;
        }
        proto.extInsert("hostname", dnsName);
        logDbg() << "Hostname =" << dnsName;
    } else {
        logDbg() << "No host information retrieved from DNS";
    }
}

} // namespace fty::scan
