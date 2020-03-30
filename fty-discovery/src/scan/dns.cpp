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
    sockaddr_in sa_in;
    sockaddr*   sa  = reinterpret_cast<sockaddr*>(&sa_in);
    socklen_t          len = sizeof(sockaddr_in);

    char dns_name[NI_MAXHOST];

    sa_in.sin_family = AF_INET;
    if (!inet_aton(address.c_str(), &sa_in.sin_addr)) {
        return;
    }

    if (!getnameinfo(sa, len, dns_name, sizeof(dns_name), nullptr, 0, NI_NAMEREQD)) {
        proto.extInsert("dns.1", dns_name);
        logDbg() << "Retrieved DNS information";
        logDbg() << "FQDN =" << dns_name;

        char* p = strchr(dns_name, '.');
        if (p) {
            *p = 0;
        }
        proto.extInsert("hostname", dns_name);
        log_debug("Hostname = '%s'", dns_name);
    } else {
        logDbg() << "No host information retrieved from DNS";
    }
}

}
