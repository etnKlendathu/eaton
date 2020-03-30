/*  =========================================================================
    range_scan - Perform one range scan

    Copyright (C) 2014 - 2017 Eaton

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

#include "scan/range.h"
#include "cidr.h"
#include "commands.h"
#include "scan/device.h"
#include "wrappers/poller.h"
#include <fty/fty-log.h>
#include <fty/split.h>

namespace fty::scan {

void RangeScan::runWorker(
    const Ranges& ranges, const DiscoveredDevices& devices, const nut::KeyValues& nutMapping)
{
    Finisher finisher([&]() {
        write(discovery::Command::Done);
    });

    if (ranges.size() < 1) {
        return;
    }

    for (auto range : ranges) {
        CIDRAddress addrcheck(range.first);
        if (!addrcheck.valid()) {
            logError() << "Address range (" << range.first << ") is not valid!";
            return;
        }
        if (addrcheck.protocol() != 4) {
            logError() << "Scanning is not supported for such range (" << range.first << ")!";
            return;
        }
    }

    std::vector<CIDRList> scans;
    for (auto range : ranges) {
        CIDRList    list;
        CIDRAddress addr;
        CIDRAddress addrDest;

        if (!range.second.empty()) {
            CIDRAddress addr_network(range.first);
            list.add(addr_network.network());
        } else {
            // real range and not subnetwork, need to scan all ips
            CIDRAddress addrStart(range.first);
            list.add(addrStart.host());
            addrDest = CIDRAddress(range.second);
            list.add(addrDest.host());
        }
        scans.push_back(list);
    }

    DeviceScan scan;
    scan.run(scans, devices, nutMapping);

    Poller poll(this, &scan);

    scan.write(discovery::Command::Scan);

    while (true) {
        auto channel = poll.wait(1000);
        if (!channel) {
            logError() << channel.error();
            break;
        }

        if (!*channel) {
            // Timeout
            continue;
        }

        if (*channel == this) {
            if (ZMessage msg = read()) {
                discovery::Command cmd = *msg.pop<discovery::Command>();
                if (cmd == discovery::Command::Term) {
                    break;
                }
            }
        } else if (*channel == &scan) {
            if (ZMessage msg = scan.read()) {
                discovery::Command cmd = *msg.pop<discovery::Command>();
                if (cmd == discovery::Command::Done) {
                    break;
                }
                msg.prepend(discovery::Command::Found);
                send(std::move(msg));
            }
        }
    }
}

} // namespace fty::scan
