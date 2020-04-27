#include "discovery/discovery-impl.h"
#include "cidr.h"
#include "wrappers/poller.h"
#include <fty/fty-log.h>
#include <fty/split.h>
#include <fty_common_db.h>
#include <fty_common_nut.h>


static fty::Expected<int64_t> computeScansSize(pack::StringList& listScan)
{
    int64_t scan_size = 0;
    for (std::string& scan : listScan) {
        size_t pos = scan.find("-");

        // if subnet
        if (pos == std::string::npos) {
            CIDRAddress addrCIDR(scan);

            if (addrCIDR.prefix() != -1) {
                logDbg() << "valid subnet" << scan;
                // all the subnet (1 << (32- prefix) ) minus subnet and broadcast address
                if (addrCIDR.prefix() <= 30)
                    scan_size += ((1 << (32 - addrCIDR.prefix())) - 2);
                else // 31/32 prefix special management
                    scan_size += (1 << (32 - addrCIDR.prefix()));
            } else {
                // not a valid range
                return fty::unexpected() << "Address subnet (" << scan << ") is not valid!";
            }
        } // else : range
        else {
            std::string rangeStart = scan.substr(0, size_t(pos));
            std::string rangeEnd   = scan.substr(size_t(pos) + 1);
            CIDRAddress addrStart(rangeStart);
            CIDRAddress addrEnd(rangeEnd);

            if (!addrStart.valid() || !addrEnd.valid() || (addrStart > addrEnd)) {
                return fty::unexpected() << scan << " is not a valid range!";
            }

            size_t      posC        = rangeStart.find_last_of(".");
            int64_t     size1       = 0;
            std::string startOfAddr = rangeStart.substr(0, posC);
            size1 += atoi(rangeStart.substr(posC + 1).c_str());

            posC = startOfAddr.find_last_of(".");
            size1 += atoi(startOfAddr.substr(posC + 1).c_str()) * 256;
            startOfAddr = startOfAddr.substr(0, posC);

            posC = startOfAddr.find_last_of(".");
            size1 += atoi(startOfAddr.substr(posC + 1).c_str()) * 256 * 256;
            startOfAddr = startOfAddr.substr(0, posC);

            posC = startOfAddr.find_last_of(".");
            size1 += atoi(startOfAddr.substr(posC + 1).c_str()) * 256 * 256 * 256;
            startOfAddr = startOfAddr.substr(0, posC);

            posC          = rangeEnd.find_last_of(".");
            int64_t size2 = 0;
            startOfAddr   = rangeEnd.substr(0, posC);
            size2 += atoi(rangeEnd.substr(posC + 1).c_str());

            posC = startOfAddr.find_last_of(".");
            size2 += atoi(startOfAddr.substr(posC + 1).c_str()) * 256;
            startOfAddr = startOfAddr.substr(0, posC);

            posC = startOfAddr.find_last_of(".");
            size2 += atoi(startOfAddr.substr(posC + 1).c_str()) * 256 * 256;
            startOfAddr = startOfAddr.substr(0, posC);

            posC = startOfAddr.find_last_of(".");
            size2 += atoi(startOfAddr.substr(posC + 1).c_str()) * 256 * 256 * 256;
            startOfAddr = startOfAddr.substr(0, posC);

            scan_size += (size2 - size1) + 1;

            pos = rangeStart.find("/");
            if (pos != std::string::npos) {
                rangeStart = rangeStart.substr(0, size_t(pos));
            }

            pos = rangeEnd.find("/");
            if (pos != std::string::npos) {
                rangeEnd = rangeEnd.substr(0, size_t(pos));
            }
            std::string correct_range = rangeStart + "/0-" + rangeEnd + "/0";
            scan                      = correct_range;

            logDbg() << "valid range (" << rangeStart << "-" << rangeEnd
                     << "). Size: " << ((size2 - size1) + 1) << PRIi64;
        }
    }

    return true;
}

int mask_nb_bit(std::string mask)
{
    size_t      pos;
    int         res = 0;
    std::string part;
    while ((pos = mask.find(".")) != std::string::npos) {
        part = mask.substr(0, pos);
        if (part == "255")
            res += 8;
        else if (part == "254")
            return res + 7;
        else if (part == "252")
            return res + 6;
        else if (part == "248")
            return res + 5;
        else if (part == "240")
            return res + 4;
        else if (part == "224")
            return res + 3;
        else if (part == "192")
            return res + 2;
        else if (part == "128")
            return res + 1;
        else if (part == "0")
            return res;
        else // error
            return -1;

        mask.erase(0, pos + 1);
    }

    if (mask == "255")
        res += 8;
    else if (mask == "254")
        res += 7;
    else if (mask == "252")
        res += 6;
    else if (mask == "248")
        res += 5;
    else if (mask == "240")
        res += 4;
    else if (mask == "224")
        res += 3;
    else if (mask == "192")
        res += 2;
    else if (mask == "128")
        res += 1;
    else if (mask == "0")
        return res;
    else
        return -1;

    return res;
}

Discovery::Impl::~Impl()
{
}

void Discovery::Impl::bind(const std::string& name)
{
    m_mlm.connect(Discovery::Endpoint, 5000, name);
    m_mlmCreate.connect(Discovery::Endpoint, 5000, name + ".create");

    m_mlm.setConsumer(FTY_PROTO_STREAM_ASSETS, ".*");

    // ask for assets now
    m_mlm.sendto("asset-agent", fty::convert<std::string>(discovery::Command::Republish), nullptr, 1000,
        ZMessage::create("$all"));
}


void Discovery::Impl::runWorker()
{
    Poller poll(this, &m_mlm);

    while (!zsys_interrupted) {
        auto which = poll.wait(5000);
        if (!which) {
            logError() << which.error();
            break;
        }

        if (*which == this) {
            if (!handlePipe(read(), poll)) {
                break; // TERM
            }
        } else if (*which == &m_mlm) {
            if (auto message = m_mlm.read()) {
                discovery::Deliver command = fty::convert<discovery::Deliver>(m_mlm.command());
                if (command == discovery::Deliver::Stream) {
                    handleStream(std::move(message));
                } else if (command == discovery::Deliver::MailBox) {
                    handleMailbox(std::move(message), poll);
                }
            }
        } else if (m_rangeScanner && *which == m_rangeScanner.get()) {
            if (ZMessage message = m_rangeScanner->read()) {
                handleRangeScanner(std::move(message), poll);
            }
        } else if (m_rangeScanConfig.size() > 0 && !m_rangeScanner) {
            if (zclock_mono() - m_assets.lastChange() > 5000) {
                // no asset change for last 5 secs => we can start range scan
                //                    log_debug("Range scanner start for %s with config file %s",
                //                        m_rangeScanConfig.ranges.front().first,
                //                        m_rangeScanConfig.config);
                // create range scanner
                // TODO: send list of IPs to skip
                reset();
                m_nbPercent = 0;
                m_percent.clear();
                rangeScannerNew();
                poll.add(m_rangeScanner.get());
            }
        }
    }
}

void Discovery::Impl::reset()
{
    m_nbDiscovered     = 0;
    m_nbEpduDiscovered = 0;
    m_nbStsDiscovered  = 0;
    m_nbUpsDiscovered  = 0;
}

bool Discovery::Impl::computeIpList(pack::StringList& listIp)
{
    for (std::string& ips : listIp) {
        CIDRAddress addrCIDR(ips, 32);
        if (addrCIDR.prefix() != -1) {
            logDbg() << "valid ip" << ips;
            ips = addrCIDR.toString();
        } else {
            // not a valid address
            logError() << "Address (" << ips << ") is not valid!";
            return false;
        }
    }
    return true;
}

void Discovery::Impl::configureLocalScan()
{
    int             s, family, prefix;
    char            host[NI_MAXHOST];
    struct ifaddrs *ifaddr, *ifa;
    std::string     addr, netm, addrmask;

    m_scanSize = 0;
    if (getifaddrs(&ifaddr) != -1) {
        for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (streq(ifa->ifa_name, "lo"))
                continue;
            if (ifa->ifa_addr == nullptr)
                continue;

            family = ifa->ifa_addr->sa_family;
            if (family != AF_INET)
                continue;

            s = getnameinfo(
                ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            if (s != 0) {
                logDbg() << "IP address parsing error for" << ifa->ifa_name << ":" << gai_strerror(s);
                continue;
            } else
                addr.assign(host);

            if (ifa->ifa_netmask == nullptr) {
                logDbg() << "No netmask found for" << ifa->ifa_name;
                continue;
            }

            family = ifa->ifa_netmask->sa_family;
            if (family != AF_INET)
                continue;

            s = getnameinfo(
                ifa->ifa_netmask, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            if (s != 0) {
                logDbg() << "Netmask parsing error for" << ifa->ifa_name << ":" << gai_strerror(s);
                continue;
            } else
                netm.assign(host);

            prefix = 0;
            addrmask.clear();

            prefix = mask_nb_bit(netm);

            // all the subnet (1 << (32- prefix) ) minus subnet and broadcast address
            if (prefix <= 30)
                m_scanSize += ((1 << (32 - prefix)) - 2);
            else // 31/32 prefix special management
                m_scanSize += (1 << (32 - prefix));


            CIDRAddress addrCidr(addr, uint(prefix));

            addrmask.clear();
            addrmask.assign(addrCidr.network().toString());

            m_localScanSubScan.push_back(addrmask);
            logInfo() << "Localscan subnet found for" << ifa->ifa_name << ":" << addrmask;
        }

        freeifaddrs(ifaddr);
    }
}

bool Discovery::Impl::computeConfigurationFile()
{
    m_defaultValuesLinks.clear();
    for (const auto& val : m_config.discovery.defaultValuesLinks) {
        link_t l;
        l.src     = val.src;
        l.dest    = 0;
        l.src_out = nullptr;
        l.dest_in = nullptr;
        l.type    = uint16_t(val.type);

        if (l.src > 0) {
            m_defaultValuesLinks.emplace_back(l);
        }
    }

    bool valid = true;

    if (m_config.discovery.scans.empty() && m_config.discovery.type == Config::Discovery::Type::MultiScan) {
        valid = false;
        logError() << "error in config file" << m_config.fileName() << ": can't have rangescan without range";
    } else if (m_config.discovery.ips.empty() && m_config.discovery.type == Config::Discovery::Type::IpScan) {
        valid = false;
        logError() << "error in config file" << m_config.fileName() << ": can't have ipscan without ip list";
    } else {
        if (m_config.discovery.type == Config::Discovery::Type::MultiScan) {
            if (auto sizeTemp = computeScansSize(m_config.discovery.scans)) {
                m_configurationScan.type      = Config::Discovery::Type::MultiScan;
                m_configurationScan.scan_size = *sizeTemp;
                m_configurationScan.scan_list = m_config.discovery.scans.value();
            } else {
                valid = false;
                logError() << sizeTemp.error();
                logError() << "Error in config file" << m_config.fileName() << ": error in range or subnet";
            }
        } else if (m_config.discovery.type == Config::Discovery::Type::IpScan) {
            if (!computeIpList(m_config.discovery.ips)) {
                valid = false;
                logError() << "Error in config file" << m_config.fileName() << ": error in ip list";
            } else {
                m_configurationScan.type      = Config::Discovery::Type::IpScan;
                m_configurationScan.scan_size = int64_t(m_config.discovery.ips.size());
                m_configurationScan.scan_list = m_config.discovery.ips.value();
            }
        } else if (m_config.discovery.type == Config::Discovery::Type::LocalScan) {
            m_configurationScan.type = Config::Discovery::Type::LocalScan;
        } else {
            valid = false;
        }
    }

    if (valid) {
        logDbg() << "config file" << m_config.fileName() << "applied successfully";
    }

    return valid;
}

void Discovery::Impl::createAsset(ZMessage&& msg)
{
    if (!msg.isFtyProto())
        return;

    FtyProto    asset = FtyProto::decode(std::move(msg));
    std::string ip    = asset.extString("ip.1");
    if (ip.empty()) {
        return;
    }

    bool daisy_chain = !asset.extString("daisy_chain").empty();
    if (!daisy_chain && m_assets.find("ip", ip)) {
        logInfo() << "Asset with IP address" << ip << "already exists";
        return;
    }

    std::string uuid = asset.extString("uuid");
    if (!uuid.empty() && m_assets.find("uuid", uuid)) {
        logInfo() << "Asset with uuid" << uuid << "already exists";
        return;
    }

    // set name
    std::string name = asset.extString("hostname");
    if (!name.empty()) {
        asset.extInsert("name", name);
    } else {
        name = asset.auxString("subtype");
        asset.extInsert("name", !name.empty() ? name + " (" + ip + ")" : ip);
    }

    if (daisy_chain) {
        std::string dc_number = asset.extString("daisy_chain");
        name                  = asset.extString("name");

        if (dc_number == "1") {
            asset.extInsert("name", name + " Host");
        } else {
            int dc_numberI = fty::convert<int>(dc_number);
            asset.extInsert("name", name + " Device" + std::to_string(dc_numberI - 1));
        }
    }

    std::time_t timestamp = std::time(nullptr);
    char        mbstr[100];
    if (std::strftime(mbstr, sizeof(mbstr), "%FT%T%z", std::localtime(&timestamp))) {
        asset.extInsert("create_ts", mbstr);
    }

    std::unique_lock<std::mutex> lock(m_devicesDiscovered.mutex());
    if (!daisy_chain) {
        if (m_devicesDiscovered.containsIp(ip)) {
            logInfo() << "Asset with IP address" << ip << "already exists";
            return;
        }
    }

    for (const auto& property : m_config.discovery.defaultValuesAux) {
        asset.extInsert(property.key, property.value);
    }

    for (const auto& property : m_config.discovery.defaultValuesExt) {
        asset.extInsert(property.key, property.value);
    }

    asset.print();
    logInfo() << "Found new asset" << asset.extString("name") << "with IP address" << ip;
    asset.setOperation("create-force");

    ZMessage assetMsg = asset.encode();
    assetMsg.prepend("READONLY");
    logDbg() << "about to send create message";

    if (auto rv = m_mlmCreate.sendto("asset-agent", "ASSET_MANIPULATION", nullptr, 10, std::move(assetMsg))) {
        logInfo() << "Create message has been sent to asset-agent (rv =" << *rv << ")";

        ZMessage response = m_mlmCreate.read();
        if (!response) {
            return;
        }

        if (auto resp = response.pop<discovery::Result>(); !resp || *resp != discovery::Result::Ok) {
            logInfo() << "Error during asset creation.";
            return;
        }

        auto resp = response.pop<std::string>();
        if (!resp) {
            logInfo() << "Error during asset creation:" << resp.error();
            return;
        }

        // create asset links
        for (auto& link : m_defaultValuesLinks) {
            if (size_t pos = resp->find("-"); pos != std::string::npos) {
                link.dest = fty::convert<uint32_t>(resp->substr(pos + 1));
            }
        }

        auto conn = tntdb::connectCached(DBConn::url);
        DBAssetsInsert::insert_into_asset_links(conn, m_defaultValuesLinks);

        m_devicesDiscovered.emplace(*resp, ip);

        name = asset.auxString("subtype", "error");
        if (name == "ups") {
            m_nbUpsDiscovered++;
        } else if (name == "epdu") {
            m_nbEpduDiscovered++;
        } else if (name == "sts") {
            m_nbStsDiscovered++;
        }
        if (name != "error") {
            m_nbDiscovered++;
        }
    } else {
        logError() << rv.error();
    }
}

void Discovery::Impl::handlePipeBind(ZMessage&& message)
{
    auto endpoint = message.pop<std::string>();
    auto myname   = message.pop<std::string>();
    assert(endpoint && myname);
    m_mlm.connect(*endpoint, 5000, *myname);
    m_mlmCreate.connect(*endpoint, 5000, *myname + ".create");
}

void Discovery::Impl::handlePipeConsumer(ZMessage&& message)
{
    auto stream  = message.pop<std::string>();
    auto pattern = message.pop<std::string>();
    assert(stream && pattern);
    m_mlm.setConsumer(*stream, *pattern);

    // ask for assets now
    ZMessage republish;
    republish.add("$all");
    m_mlm.sendto("asset-agent", fty::convert<std::string>(discovery::Command::Republish), nullptr, 1000,
        std::move(republish));
}

void Discovery::Impl::handlePipeConfig(ZMessage&& message)
{
    auto config = message.pop<std::string>();
    m_config.load(*config);

    if (!m_config.hasValue()) {
        logError() << "failed to load config file" << *config;
    }

    bool valid = true;

    if (m_config.discovery.scans.empty() && m_config.discovery.type == Config::Discovery::Type::MultiScan) {
        valid = false;
        logError() << "error in config file" << *config << ": can't have rangescan without range";
    } else if (m_config.discovery.ips.empty() && m_config.discovery.type == Config::Discovery::Type::IpScan) {
        valid = false;
        logError() << "error in config file " << *config << ": can't have ipscan without ip list";
    } else {
        auto sizeTemp = computeScansSize(m_config.discovery.scans);
        if (sizeTemp && computeIpList(m_config.discovery.ips)) {
            // ok, validate the config
            m_configurationScan.type      = m_config.discovery.type;
            m_configurationScan.scan_size = *sizeTemp;
            m_configurationScan.scan_list.clear();
            m_configurationScan.scan_list = m_config.discovery.scans.value();

            if (m_configurationScan.type == Config::Discovery::Type::IpScan) {
                m_configurationScan.scan_size = m_config.discovery.ips.size();
                m_configurationScan.scan_list.clear();
                m_configurationScan.scan_list = m_config.discovery.ips.value();
            }
        } else {
            valid = false;
            logError() << "error in config file" << *config << ": error in scans";
        }
    }

    std::string mappingPath = m_config.parameters.mappingFile;
    if (mappingPath == "none") {
        logError() << "No mapping file declared under config key '" << m_config.parameters.mappingFile.key()
                   << "'";
        valid = false;
    } else {
        try {
            m_nutMappingInventory = fty::nut::loadMapping(mappingPath, "inventoryMapping");
            logInfo() << "Mapping file '" << mappingPath << "' loaded," << m_nutMappingInventory.size()
                      << "inventory mappings";
        } catch (const std::exception& e) {
            logError() << "Couldn't load mapping file '" << mappingPath << "': " << e.what();
            valid = false;
        }
    }

    if (valid) {
        logDbg() << "config file" << *config << "applied successfully";
    }
}

void Discovery::Impl::handlePipeScan(ZMessage&& message, Poller& poll)
{
    if (m_rangeScanner) {
        reset();
        poll.remove(m_rangeScanner.get());
        m_rangeScanner.reset();
    }
    m_ongoingStop = false;
    m_localScanSubScan.clear();
    m_scanSize = 0;

    m_rangeScanConfig.emplace_back(*message.pop<std::string>(), nullptr);
    if (!m_rangeScanConfig[0].first.empty()) {
        CIDRAddress addrCIDR((m_rangeScanConfig[0]).first);

        if (addrCIDR.prefix() != -1) {
            // all the subnet (1 << (32- prefix) ) minus subnet and broadcast address
            if (addrCIDR.prefix() <= 30)
                m_scanSize = ((1 << (32 - addrCIDR.prefix())) - 2);
            else // 31/32 prefix special management
                m_scanSize = (1 << (32 - addrCIDR.prefix()));
        } else {
            // not a valid range
            logError() << "Address range (" << m_rangeScanConfig[0].first << ") is not valid!";
        }
    }
}

void Discovery::Impl::handlePipeLocalScan(Poller& poll)
{
    if (m_rangeScanner) {
        reset();
        poll.remove(m_rangeScanner.get());
        m_rangeScanner.reset();
    }

    m_ongoingStop = false;
    m_localScanSubScan.clear();
    m_scanSize = 0;
    configureLocalScan();

    m_rangeScanConfig.clear();
    if (m_scanSize > 0) {
        while (m_localScanSubScan.size() > 0) {
            zmsg_t* zmfalse = zmsg_new();
            zmsg_addstr(zmfalse, m_localScanSubScan.back().c_str());

            char* secondnullptr = nullptr;
            m_rangeScanConfig.push_back(std::make_pair(zmsg_popstr(zmfalse), secondnullptr));
            m_localScanSubScan.pop_back();
            zmsg_destroy(&zmfalse);
        }

        reset();
    }
}

bool Discovery::Impl::handlePipe(ZMessage&& message, Poller& poller)
{
    auto command = message.pop<discovery::Command>();
    if (!command) {
        logWarn() << command.error();
        return true;
    }
    logDbg() << "handlePipe DO:" << *command;

    if (*command == discovery::Command::Term) {
        logInfo() << "Got $TERM";
        return false;
    } else if (*command == discovery::Command::Bind) {
        handlePipeBind(std::move(message));
    } else if (*command == discovery::Command::Consumer) {
        handlePipeConsumer(std::move(message));
    } else if (*command == discovery::Command::Config) {
        handlePipeConfig(std::move(message));
    } else if (*command == discovery::Command::Scan) {
        handlePipeScan(std::move(message), poller);
    } else if (*command == discovery::Command::LocalScan) {
        handlePipeLocalScan(poller);
    } else {
        logError() << "s_handle_pipe: Unknown command: " << *command << ".\n";
    }
    return true;
}

void Discovery::Impl::launchScan(ZMessage&& msg, Poller& poller)
{
    auto     zuuid = msg.pop<std::string>();
    ZMessage reply;
    reply.add(*zuuid);

    if (m_rangeScanner) {
        reply.add(m_ongoingStop ? discovery::Status::Stopping : discovery::Status::Running);
    } else {
        m_ongoingStop = false;
        m_percent     = "0";

        m_localScanSubScan.clear();
        m_scanSize  = 0;
        m_nbPercent = 0;

        if (computeConfigurationFile()) {
            if (m_configurationScan.type == Config::Discovery::Type::LocalScan) {
                // Launch localScan
                configureLocalScan();

                if (m_scanSize > 0) {
                    while (m_localScanSubScan.size() > 0) {
                        m_rangeScanConfig.push_back({m_localScanSubScan.back(), nullptr});

                        logDbg() << "Range scanner requested for" << m_rangeScanConfig.back().first
                                 << "with config file" << m_config.fileName();
                        m_localScanSubScan.pop_back();
                    }
                    // create range scanner
                    if (m_rangeScanner) {
                        poller.remove(m_rangeScanner.get());
                        m_rangeScanner.reset();
                    }
                    reset();
                    rangeScannerNew();
                    poller.add(m_rangeScanner.get());
                    m_statusScan = discovery::Status::Progress;

                    reply.add(discovery::Result::Ok);
                } else {
                    reply.add(discovery::Result::Error);
                }

            } else if ((m_configurationScan.type == Config::Discovery::Type::MultiScan) ||
                       (m_configurationScan.type == Config::Discovery::Type::IpScan)) {
                // Launch rangeScan
                m_localScanSubScan = m_configurationScan.scan_list;
                m_scanSize         = m_configurationScan.scan_size;

                while (m_localScanSubScan.size() > 0) {
                    std::string next_scan = m_localScanSubScan.back();
                    auto [first, last]    = fty::split<std::string, std::string>(next_scan, "-");
                    m_rangeScanConfig.emplace_back(first, last);

                    logDbg() << "Range scanner requested for" << next_scan << "with config file"
                             << m_config.fileName();

                    m_localScanSubScan.pop_back();
                }

                // create range scanner
                if (m_rangeScanner) {
                    poller.remove(m_rangeScanner.get());
                    m_rangeScanner.reset();
                }
                reset();
                rangeScannerNew();
                poller.add(m_rangeScanner.get());

                m_statusScan = discovery::Status::Progress;
                reply.add(discovery::Result::Ok);
            } else {
                reply.add(discovery::Result::Error);
            }
        } else {
            reply.add(discovery::Result::Error);
        }
    }
    m_mlm.sendto(m_mlm.sender(), m_mlm.subject(), m_mlm.tracker(), 1000, std::move(reply));
}

void Discovery::Impl::handleProgress(ZMessage&& msg)
{
    auto     zuuid = msg.pop<std::string>();
    ZMessage reply;
    reply.add(*zuuid);
    if (!m_percent.empty()) {
        reply.add(discovery::Result::Ok);
        reply.add(fty::convert<std::string>(m_statusScan) + PRIi32);
        reply.add(m_percent);
        reply.add(fty::convert<std::string>(m_nbDiscovered) + PRIi64);
        reply.add(fty::convert<std::string>(m_nbUpsDiscovered) + PRIi64);
        reply.add(fty::convert<std::string>(m_nbEpduDiscovered) + PRIi64);
        reply.add(fty::convert<std::string>(m_nbStsDiscovered) + PRIi64);
    } else {
        reply.add(discovery::Result::Ok);
        reply.add("-1" PRIi32);
    }
    m_mlm.sendto(m_mlm.sender(), m_mlm.subject(), m_mlm.tracker(), 1000, std::move(reply));
}

void Discovery::Impl::handleStopScan(ZMessage&& msg)
{
    auto     zuuid = msg.pop<std::string>();
    ZMessage reply;
    reply.add(*zuuid);
    reply.add(discovery::Result::Ok);

    m_mlm.sendto(m_mlm.sender(), m_mlm.subject(), m_mlm.tracker(), 1000, std::move(reply));
    if (m_rangeScanner && !m_ongoingStop) {
        m_statusScan  = discovery::Status::Stopped;
        m_ongoingStop = true;
        m_rangeScanner->write(fty::convert<std::string>(discovery::Command::Term));
    }

    m_localScanSubScan.clear();
    m_scanSize = 0;
}

void Discovery::Impl::handleMailbox(ZMessage&& msg, Poller& poller)
{
    if (msg.isFtyProto()) {
        FtyProto proto = FtyProto::decode(std::move(msg));
        m_assets.put(std::move(proto));
    } else {
        // handle REST API requests
        auto cmd = msg.pop<discovery::Command>();
        if (!cmd) {
            logWarn() << "handleMailbox:" << cmd.error();
            return;
        }

        logDbg() << "handleMailbox DO:" << *cmd;

        switch (*cmd) {
            case discovery::Command::LaunchScan:
                launchScan(std::move(msg), poller);
                break;
            case discovery::Command::Progress:
                handleProgress(std::move(msg));
                break;
            case discovery::Command::StopScan:
                handleStopScan(std::move(msg));
                break;
            default:
                logError() << "s_handle_mailbox: Unknown actor command:" << *cmd << ".\n";
        }
    }
}


//  --------------------------------------------------------------------------
//  process message stream

void Discovery::Impl::handleStream(ZMessage&& message)
{
    if (!message.isFtyProto()) {
        return;
    }
    // handle fty_proto protocol here
    FtyProto fmsg = FtyProto::decode(std::move(message));

    if (fmsg && (fmsg.id() == FTY_PROTO_ASSET)) {
        std::string operation = fmsg.operation();
        // TODO : Remove as soon as we can this ugly hack of "_no_not_really"
        if (operation == FTY_PROTO_ASSET_OP_DELETE) {
            auto aux = fmsg.aux();
            if (aux.find("_no_not_really") != aux.end()) {
                m_devicesDiscovered.remove(fmsg.name());
            }
        } else if (operation == FTY_PROTO_ASSET_OP_CREATE || operation == FTY_PROTO_ASSET_OP_UPDATE) {
            std::string ip = fmsg.extString("ip.1");
            if (!ip.empty()) {
                m_devicesDiscovered.emplace(fmsg.name(), ip);
            }
        }
    }
}

//  --------------------------------------------------------------------------
//  process message stream
//  * DONE
//  * FOUND
//  * POOGRESS

void Discovery::Impl::handleRangeScanner(ZMessage&& msg, Poller& poll)
{
    msg.print();
    auto cmd = msg.pop<discovery::Command>();
    if (!cmd) {
        logWarn() << "handleRangeScanner:" << cmd.error();
        return;
    }
    logDbg() << "handleRangeScanner:" << *cmd;
    if (*cmd == discovery::Command::Done) {
        if (m_ongoingStop && m_rangeScanner) {
            poll.remove(m_rangeScanner.get());
            m_rangeScanner.reset();
        } else {
            write(discovery::Command::Done);
            if (!m_localScanSubScan.empty()) {
                m_localScanSubScan.clear();
            }

            m_statusScan = discovery::Status::Finished;
            m_rangeScanConfig.clear();

            poll.remove(m_rangeScanner.get());
            m_rangeScanner.reset();
        }
    } else if (*cmd == discovery::Command::Found) {
        createAsset(std::move(msg));
    } else if (*cmd == discovery::Command::Progress) {
        ++m_nbPercent;
        m_percent = std::to_string(m_nbPercent * 100 / m_scanSize);
        // other cmd. NOTFOUND must not be treated
    } else if (*cmd != discovery::Command::NotFound) {
        logError() << "handleRangeScanner: Unknown  command:" << *cmd << ".\n";
    }
}

void Discovery::Impl::rangeScannerNew()
{
    m_rangeScanner = std::unique_ptr<fty::scan::RangeScan>();
    m_rangeScanner->run(m_rangeScanConfig, m_devicesDiscovered, m_nutMappingInventory);
}

// ===========================================================================================================

const std::string& DiscoveryConfig::fileName() const
{
    return m_fileName;
}

void DiscoveryConfig::load(const std::string& file)
{
    clear();
    pack::zconfig::deserializeFile(file, *this);
    m_fileName = file;
}
