/*  =========================================================================
    device_scan - Perform one IP address scan

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

#include "scan/device.h"
#include "commands.h"
#include "scan/nut.h"
#include "wrappers/poller.h"
#include <algorithm>
#include <czmq.h>
#include <fty/fty-log.h>
#include "discovery/serverconfig.h"
#include "discovery/discovered-devices.h"

namespace fty::scan {

bool DeviceScan::scanDevices(const std::vector<CIDRList>& list, const DiscoveredDevices& devices,
    const nut::KeyValues& nutMapping, const pack::StringList& docs)
{
    std::vector<Nut> actors;
    Poller           poll(this);
    for (const auto& toScan : list) {
        Nut& actor = actors.emplace_back();
        actor.run(toScan, devices, nutMapping, docs);
        poll.add(&actor);
    }

    bool   term             = false;
    size_t number_end_actor = 0;

    while (!zsys_interrupted) {
        Expected<IPipe*> which = poll.wait(5000);
        if (!which) {
            break;
        }

        if (!*which) {
            // timeout
            continue;
        }

        if (*which == this) {
            if (ZMessage msg = read()) {
                auto cmd = *msg.pop<discovery::Command>();
                if (cmd == discovery::Command::Term) {
                    term = true;
                    break;
                }
            }
        } else {
            // any of scan_nut_actor
            ZMessage msg = (*which)->read();
            if (!msg) {
                continue;
            }

            auto cmd = *msg.pop<discovery::Command>();
            if (cmd == discovery::Command::Term) {
                break;
            } else if (cmd == discovery::Command::Done) {
                poll.remove(*which);
                auto actorIt = std::find_if(actors.begin(), actors.end(), [&](const Nut& nut) {
                    return &nut == *which;
                });
                if (actorIt == actors.end()) {
                    // ERROR ? Normaly can't happened
                    logError() << __FUNCTION__ << "Error : actor not in the actor list";
                } else {
                    actors.erase(actorIt);
                }

                if (actors.empty()) {
                    break;
                } else if (number_end_actor >= actors.size()) {
                    for (auto& actor : actors) {
                        actor.write(discovery::Command::Continue);
                    }
                    number_end_actor = 0;
                }
            } else if (cmd == discovery::Command::InfoReady) {
                number_end_actor++;
                if (number_end_actor >= actors.size()) {
                    for (auto& actor : actors) {
                        actor.write(discovery::Command::Continue);
                    }
                    number_end_actor = 0;
                }
            } else if (cmd == discovery::Command::Found) {
                msg.prepend(discovery::Command::Found);
                send(std::move(msg));
            }
        }
    }

    logDbg() << "QUIT device scan scan";
    return term;
}

//  --------------------------------------------------------------------------
//  One device scan actor

void DeviceScan::runWorker(
    const std::vector<CIDRList>& list, const DiscoveredDevices& devices, const nut::KeyValues& nutMapping)
{
    zsock_signal(pipe(), 0);


    logDbg() << "dsa: device scan actor created";
    while (!zsys_interrupted) {
        ZMessage msg = read();
        if (!msg) {
            continue;
        }

        auto cmd = *msg.pop<discovery::Command>();
        if (cmd == discovery::Command::Term) {
            break;
        } else if (cmd == discovery::Command::Scan) {
            ServerConfig& conf = ServerConfig::instance();

            const int             numberMaxPool = conf.parameters.maxScanPoolNumber;
            std::vector<CIDRList> pool;
            for (const auto& scan : list) {
                pool.push_back(scan);
                if (int(pool.size()) == numberMaxPool) {
                    if (!scanDevices(pool, devices, nutMapping, conf.discovery.documents)) {
                        pool.clear();
                        break;
                    }
                }
            }
            if (!pool.empty()) {
                scanDevices(pool, devices, nutMapping, conf.discovery.documents);
            }

            write(discovery::Command::Done);
            break;
        }
    }
    logDbg() << "dsa: device scan actor exited";
}

} // namespace fty::scan
