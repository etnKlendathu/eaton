/*  =========================================================================
    scan_nut - collect information from DNS

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

#include "scan/nut.h"
#include "commands.h"
#include "server.h"
#include "scan/dns.h"
#include "wrappers/ftyproto.h"
#include "wrappers/poller.h"
#include <algorithm>
#include <fty/fty-log.h>
#include <fty/split.h>
#include <fty_common_nut.h>
#include <fty_security_wallet.h>
#include "discovery/discovered-devices.h"
#include "discovery/serverconfig.h"

static const std::string SECW_SOCKET_PATH = "/run/fty-security-wallet/secw.socket";
#define BIOS_NUT_DUMPDATA_ENV "BIOS_NUT_DUMPDATA"

namespace fty::scan {

struct ScanResult
{
    ScanResult(const std::string& driver, const std::vector<secw::DocumentPtr>& docs = {})
        : nutDriver(driver)
        , documents(docs)
    {
    }

    std::string                    nutDriver;
    std::vector<secw::DocumentPtr> documents;
    nut::DeviceConfigurations      deviceConfigurations;
};

struct NutOutput
{
    std::string port;
    std::string ip;
};

// parse nut config line (key = "value")
//static std::pair<std::string, std::string> keyAndValue(const std::string& line)
//{
//    static std::regex re("([a-zA-Z0-9]+)\\s*=\\s*\"([^\"]+)\"");
//    auto [key, value] = split<std::string, std::string>(line, re);
//    return {key, value};
//}

static void nutOutputToMessages(
    std::vector<NutOutput>& assets, const nut::DeviceConfigurations& output, const DiscoveredDevices& devices)
{
    for (const auto& device : output) {
        bool      found = false;
        NutOutput asset;

        const auto itPort = device.find("port");
        if (itPort != device.end()) {
            std::string ip;

            if (size_t pos = itPort->second.find("://"); pos != std::string::npos) {
                ip = itPort->second.substr(pos + 3);
            } else {
                ip = itPort->second;
            }

            if (devices.containsIp(ip)) {
                found = false;
                break;
            } else {
                asset.ip   = ip;
                asset.port = itPort->second;
                found      = true;
            }
        }

        if (found) {
            assets.push_back(asset);
        }
    }
}

static bool validDumpdata(const nut::DeviceConfiguration& dump)
{
    if (dump.find("device.type") == dump.end()) {
        logError() << "No subtype for this device";
        return false;
    }

    if (dump.find("device.model") == dump.end() && dump.find("ups.model") == dump.end() &&
        dump.find("device.1.model") == dump.end() && dump.find("device.1.ups.model") == dump.end()) {
        logError() << "No model for this device";
        return false;
    }

    if (dump.find("device.mfr") == dump.end() && dump.find("ups.mfr") == dump.end() &&
        dump.find("device.1.mfr") == dump.end() && dump.find("device.1.ups.mfr") == dump.end()) {
        logError() << "No subtype for this device";
        return false;
    }

    return true;
}


static bool nutDumpdataToFtyMessage(std::vector<FtyProto>& assets, const nut::DeviceConfiguration& dump,
    const nut::KeyValues& mappings, const std::string& ip, const std::string& type)
{
    // Set up iteration limits according to daisy-chain configuration.
    int startDevice = 0, endDevice = 0;
    {
        auto item = dump.find("device.count");
        if (item != dump.end() && item->second != "1") {
            startDevice = 1;
            endDevice   = convert<int>(item->second);
        }
    }

    for (int i = startDevice; i <= endDevice; ++i) {
        FtyProto proto(FTY_PROTO_ASSET);

        // Map inventory data.
        auto mappedDump = nut::performMapping(mappings, dump, i);
        for (auto property : mappedDump) {
            proto.extInsert(property.first, property.second);
        }

        // Try to obtain DNS name ("hostname" + "dns.1" attributes)
        Dns::scanDns(proto, ip);

        // Some special cases.
        proto.extInsert("ip.1", ip);
        proto.extInsert("type", type);

        if (i != 0) {
            proto.extInsert("daisy_chain", convert<std::string>(i));
        }

        if (proto.extString("manufacturer").empty() || proto.extString("model").empty()) {
            logError() << "No manufacturer or model for device number" << i;
            continue;
        }

        {
            // Getting type from dump is safer than parsing driver name.
            auto item = dump.find("device.type");
            if (item != dump.end()) {
                std::string device = item->second;
                if (device == "pdu") {
                    device = "epdu";
                } else if (device == "ats") {
                    device = "sts";
                }
                proto.extInsert("subtype", device);
            }
        }

        assets.emplace_back(std::move(proto));
    }

    return !assets.empty();
}

class DumpActor : public Actor<DumpActor>
{
    static constexpr uint DefaultDumpDataLoop = 2;
public:
    void runWorker(const NutOutput& output, const ScanResult& result, const nut::KeyValues& mappings)
    {
        uint loopNb = DefaultDumpDataLoop;
        if (::getenv(BIOS_NUT_DUMPDATA_ENV)) {
            loopNb = convert<uint>(::getenv(BIOS_NUT_DUMPDATA_ENV));
        }

        uint loopIterTime = ServerConfig::instance().parameters.dumpDataLoopTime;

        // waiting message from caller
        // it avoid Assertion failed: pfd.revents & POLLIN (signaler.cpp:242) on zpoller_wait for
        // a pool of zactor who use Subprocess
        if (informAndWait()) {
            return;
        }

        const std::string addr = output.port;
        const std::string ip   = output.ip;
        const std::string type = "device";

        nut::DeviceConfiguration nutdata =
            nut::dumpDevice(result.nutDriver, addr, loopNb, loopIterTime, result.documents);

        ZMessage reply;
        if (!nutdata.empty()) {
            std::vector<FtyProto> assets;
            if (validDumpdata(nutdata) && nutDumpdataToFtyMessage(assets, nutdata, mappings, ip, type)) {
                logDbg() << "Dump data for" << addr << "(" << result.nutDriver << ") succeeded.";

                size_t size = assets.size();
                for (const FtyProto& proto : assets) {
                    ZMessage enc = proto.encode();
                    reply.add(--size == 0 ? discovery::Command::Found : discovery::Command::FoundDc);
                    send(std::move(enc));
                }
            } else {
                logDbg() << "Dump data for" << addr << "(" << result.nutDriver << ") failed: invalid data.";
                reply.add(discovery::Result::Failed);
            }
        } else {
            logDbg() << "Dump data for" << addr << "(" << result.nutDriver
                     << ") failed: failed to dump data.";
            reply.add(discovery::Result::Failed);
        }

        if (reply) {
            send(std::move(reply));
        }

        bool stop = false;
        while (!stop && !zsys_interrupted) {
            if (ZMessage msgStop = read()) {
                auto cmd = *msgStop.pop<discovery::Command>();
                if (cmd == discovery::Command::Term) {
                    stop = true;
                }
            }
        }
    }
};


bool Nut::createPoolDumpdata(
    const ScanResult& result, const DiscoveredDevices& devices, const nut::KeyValues& mappings)
{
    Poller                 poll(this);
    std::vector<DumpActor> actors;

    std::vector<NutOutput> listDiscovered;
    nutOutputToMessages(listDiscovered, result.deviceConfigurations, devices);

    int number_max_pool = ServerConfig::instance().parameters.maxDumpPoolNumber;

    bool stop_now = false;

    size_t number_asset_view = 0;
    while (number_asset_view < listDiscovered.size()) {
        if (askActorTerm()) {
            stop_now = true;
        }

        if (zsys_interrupted || stop_now) {
            break;
        }

        int number_pool = 0;
        while (number_pool < number_max_pool && number_asset_view < listDiscovered.size()) {
            auto& asset = listDiscovered.at(number_asset_view);
            number_asset_view++;

            DumpActor actor = actors.emplace_back();
            actor.run(asset, result, mappings);

            if (ZMessage msgReady = actor.read()) {
                auto cmd = *msgReady.pop<discovery::Command>();
                if (cmd == discovery::Command::InfoReady) {
                    number_pool++;
                    poll.add(&actor);
                }
            }
        }

        // All subactor createdfor this one, inform and wait
        write(discovery::Command::InfoReady);
        // wait
        stop_now = true;
        if (ZMessage msgRun = read()) {
            auto cmd = *msgRun.pop<discovery::Command>();
            if (cmd == discovery::Command::Continue) {
                stop_now = false;
                // All subactor created, they can continue
                for (auto& actor : actors) {
                    actor.write(discovery::Command::Continue);
                }
            }
        }

        int count = 0;
        while (count < number_pool) {
            if (zsys_interrupted || stop_now) {
                stop_now = true;
                break;
            }
            Expected<IPipe*> which = poll.wait(-1);
            if (!which) {
                logError() << which.error();
                stop_now = true;
                break;
            }
            if (!*which) {
                logDbg() << "Error on create_pool_dumpdata";
                stop_now = true;
                break;
            }

            if (ZMessage msgRec = which.value()->read()) {
                auto cmd = *msgRec.pop<discovery::Command>();
                if (*which == this) {
                    if (cmd == discovery::Command::Term) {
                        stop_now = true;
                    }
                } else {
                    count++;
                    if (cmd == discovery::Command::Found) {
                        poll.remove(*which);
                        msgRec.prepend(discovery::Command::Found);
                        send(std::move(msgRec));
                    } else if (cmd == discovery::Command::FoundDc) {
                        msgRec.prepend(discovery::Command::Found);
                        send(std::move(msgRec));
                        count--;
                    } else { // Dump failed
                        poll.remove(*which);
                    }
                }
            }
        }
    }
    return stop_now;
}


//  --------------------------------------------------------------------------
//  Scan IPs addresses using nut-scanner
void Nut::runWorker(const CIDRList& list, const DiscoveredDevices& devices, const nut::KeyValues& nutMapping,
    const pack::StringList& docs)
{
    Finisher finisher([&]() {
        write(discovery::Command::Done);
    });

    if (list.empty() || devices.empty() || nutMapping.empty() || docs.empty()) {
        logError() << __FUNCTION__ << ": actor created without config or devices list";
        write(discovery::Command::Done);
        return;
    }

    std::vector<ScanResult>        results;
    std::vector<secw::DocumentPtr> credentialsV3;
    std::vector<secw::DocumentPtr> credentialsV1;

    // Grab security documents.
    try {
        SocketSyncClient secwSyncClient(SECW_SOCKET_PATH);

        auto client   = secw::ConsumerAccessor(secwSyncClient);
        auto secCreds = client.getListDocumentsWithPrivateData("default", "discovery_monitoring");

        for (const secw::DocumentPtr& doc : secCreds) {
            if (!docs.find(doc->getId())) {
                continue;
            }

            if (auto credV3 = secw::Snmpv3::tryToCast(doc)) {
                credentialsV3.emplace_back(doc);
            } else if (auto credV1 = secw::Snmpv1::tryToCast(doc)) {
                credentialsV1.emplace_back(doc);
            }
        }
        logDbg() << "Fetched" << credentialsV3.size() << "SNMPv3 and" << credentialsV1.size()
                 << "SNMPv1 credentials from security wallet.";
    } catch (const std::exception& e) {
        logWarn() << "Failed to fetch credentials from security wallet:" << e.what();
    }

    // Grab timeout.
    uint32_t timeout = ServerConfig::instance().parameters.nutScannerTimeOut;

    // Scan the network and store credential/protocol pairs which returned data.
    // SNMPv3 scan.
    for (const secw::DocumentPtr& credential : credentialsV3) {
        ScanResult result("snmp-ups", {credential});

        result.deviceConfigurations =
            nut::scanRangeDevices(nut::SCAN_PROTOCOL_SNMP, list.firstAddress().toString(CIDR_WITHOUT_PREFIX),
                list.lastAddress().toString(CIDR_WITHOUT_PREFIX), timeout, result.documents);

        results.emplace_back(result);
    }

    // SNMPv1 scan.
    for (const secw::DocumentPtr& credential : credentialsV1) {
        ScanResult result("snmp-ups", {credential});

        result.deviceConfigurations =
            nut::scanRangeDevices(nut::SCAN_PROTOCOL_SNMP, list.firstAddress().toString(CIDR_WITHOUT_PREFIX),
                list.lastAddress().toString(CIDR_WITHOUT_PREFIX), timeout, result.documents);

        results.emplace_back(result);
    }

    // NetXML scan.
    {
        ScanResult result("netxml-ups");

        result.deviceConfigurations = nut::scanRangeDevices(nut::SCAN_PROTOCOL_NETXML,
            list.firstAddress().toString(CIDR_WITHOUT_PREFIX),
            list.lastAddress().toString(CIDR_WITHOUT_PREFIX), timeout);

        results.emplace_back(result);
    }

    bool stop_now = false;

    if (askActorTerm()) {
        stop_now = true;
    }

    if (zsys_interrupted || stop_now) {
        return;
    }

    stop_now = informAndWait();

    if (zsys_interrupted || stop_now) {
        return;
    }

    for (const auto& result : results) {
        stop_now = createPoolDumpdata(result, devices, nutMapping);

        if (askActorTerm()) {
            stop_now = true;
        }
        if (zsys_interrupted || stop_now) {
            break;
        }
    }

    logDbg() << "scan nut actor exited";
}

} // namespace fty::scan
