/*  =========================================================================
    fty_discovery_server - Manages discovery requests, provides feedback

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

/*
@header
    fty_discovery_server - Manages discovery requests, provides feedback
@discuss
@end
 */

#include "fty_discovery_classes.h"
#include "wrappers/mlm.h"
#include "wrappers/zmessage.h"
#include <ctime>
#include <fty/expected.h>
#include <fty/fty-log.h>
#include <fty/split.h>
#include <ifaddrs.h>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <vector>

Config& discoveryConfig()
{
    static Config cfg = pack::zconfig::deserializeFile<Config>(FTY_DISCOVERY_CFG_FILE);
    return cfg;
}

//  Structure of our class

typedef struct _configuration_scan_t
{
    std::vector<std::string> scan_list;
    int64_t                  scan_size;
    Config::Discovery::Type  type;
} configuration_scan_t;


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


//  --------------------------------------------------------------------------
//  fty_discovery_server actor

#if 0
void fty_discovery_server(zsock_t* pipe, void* /*args*/)
{
    fty_discovery_server_t* self   = fty_discovery_server_new();
    zpoller_t*              poller = zpoller_new(pipe, self->mlm.msgpipe(), nullptr);
    zsock_signal(pipe, 0);
    zmsg_t* range_stack = zmsg_new();

    while (!zsys_interrupted) {
        void* which = zpoller_wait(poller, 5000);
        if (which == pipe) {
            if (!s_handle_pipe(self, zmsg_recv(pipe), poller))
                break; // TERM
        } else if (which == self->mlm.msgpipe()) {
            auto message = self->mlm.recv();
            if (!message)
                continue;
            std::string command = self->mlm.command();
            if (command == STREAM_CMD) {
                s_handle_stream(self, *message);
            } else if (command == MAILBOX_CMD) {
                s_handle_mailbox(self, *message, poller);
            }
        } else if (self->range_scanner && which == self->range_scanner) {
            zmsg_t* message = zmsg_recv(self->range_scanner);
            if (!message)
                continue;
            s_handle_range_scanner(self, message, poller, pipe);
        }
        // check that scanner is nullptr && we have to do scan
        if (self->range_scan_config.ranges.size() > 0 && !self->range_scanner) {
            if (zclock_mono() - assets_last_change(self->assets) > 5000) {
                // no asset change for last 5 secs => we can start range scan
                log_debug("Range scanner start for %s with config file %s",
                    self->range_scan_config.ranges.front().first, self->range_scan_config.config);
                // create range scanner
                // TODO: send list of IPs to skip
                reset_nb_discovered(self);
                self->nb_percent = 0;
                if (self->percent)
                    zstr_free(&self->percent);
                self->range_scanner = range_scanner_new(self);
                zpoller_add(poller, self->range_scanner);
            }
        }
    }
    zmsg_destroy(&range_stack);
    fty_discovery_server_destroy(&self);
    zpoller_destroy(&poller);
}

//  --------------------------------------------------------------------------
//  Create a new fty_discovery_server

fty_discovery_server_t* fty_discovery_server_new()
{
    fty_discovery_server_t* self = (fty_discovery_server_t*)zmalloc(sizeof(fty_discovery_server_t));
    assert(self);
    //  Initialize class properties here
    self->scanner                        = nullptr;
    self->assets                         = assets_new();
    self->nb_discovered                  = 0;
    self->nb_epdu_discovered             = 0;
    self->nb_sts_discovered              = 0;
    self->nb_ups_discovered              = 0;
    self->scan_size                      = 0;
    self->ongoing_stop                   = false;
    self->status_scan                    = -1;
    self->range_scan_config.config       = strdup(FTY_DISCOVERY_CFG_FILE);
    self->configuration_scan.type        = TYPE_LOCALSCAN;
    self->configuration_scan.scan_size   = 0;
    self->devices_discovered.device_list = zhash_new();
    self->percent                        = nullptr;
    self->nut_mapping_inventory          = fty::nut::KeyValues();
    self->default_values_aux             = std::map<std::string, std::string>();
    self->default_values_ext             = std::map<std::string, std::string>();
    self->default_values_links           = std::vector<link_t>();
    return self;
}

void fty_discovery_server_destroy(fty_discovery_server_t** self_p)
{
    assert(self_p);
    if (*self_p) {
        fty_discovery_server_t* self = *self_p;
        //  Free class properties here
        zactor_destroy(&self->scanner);
        assets_destroy(&self->assets);
        if (self->range_scan_config.config)
            zstr_free(&self->range_scan_config.config);
        if (self->range_scan_config.ranges.size() > 0) {
            for (auto range : self->range_scan_config.ranges) {
                zstr_free(&(range.first));
                zstr_free(&(range.second));
            }
            self->range_scan_config.ranges.clear();
        }
        self->range_scan_config.ranges.shrink_to_fit();
        if (self->percent)
            zstr_free(&self->percent);
        if (self->range_scanner)
            zactor_destroy(&self->range_scanner);
        if (self->devices_discovered.device_list)
            zhash_destroy(&self->devices_discovered.device_list);
        // FIXME: I, feel something so wrong / Doing the right thing...
        self->configuration_scan.scan_list.~vector();
        self->localscan_subscan.~vector();
        self->nut_mapping_inventory.~map();
        self->default_values_aux.~map();
        self->default_values_ext.~map();
        self->default_values_links.~vector();
        //  Free object itself
        free(self);
        *self_p = nullptr;
    }
}
#endif

class Discovery::Impl : public ActorImpl
{
public:
    void worker(zsock_t* sock) override
    {
        zpoller_t*              poller = zpoller_new(sock, m_mlm.msgpipe(), nullptr);
        zsock_signal(sock, 0);
        zmsg_t* range_stack = zmsg_new();

        while (!zsys_interrupted) {
            void* which = zpoller_wait(poller, 5000);
            if (which == sock) {
                if (!handlePipe(zmsg_recv(sock), poller))
                    break; // TERM
            } else if (which == m_mlm.msgpipe()) {
                auto message = m_mlm.recv();
                if (!message)
                    continue;
                std::string command = m_mlm.command();
                if (command == STREAM_CMD) {
                    handleStream(*message);
                } else if (command == MAILBOX_CMD) {
                    handleMailbox(*message, poller);
                }
            } else if (m_rangeScanner && which == m_rangeScanner) {
                zmsg_t* message = zmsg_recv(m_rangeScanner);
                if (!message)
                    continue;
                handleRangeScanner(message, poller, sock);
            }
            // check that scanner is nullptr && we have to do scan
            if (m_rangeScanConfig.ranges.size() > 0 && !m_rangeScanner) {
                if (zclock_mono() - m_assets.lastChange() > 5000) {
                    // no asset change for last 5 secs => we can start range scan
//                    log_debug("Range scanner start for %s with config file %s",
//                        m_rangeScanConfig.ranges.front().first, m_rangeScanConfig.config);
                    // create range scanner
                    // TODO: send list of IPs to skip
                    reset();
                    m_nbPercent = 0;
                    m_percent.clear();
                    m_rangeScanner = rangeScannerNew();
                    zpoller_add(poller, m_rangeScanner);
                }
            }
        }
        zmsg_destroy(&range_stack);
        zpoller_destroy(&poller);
    }

private:
    void reset()
    {
        m_nbDiscovered     = 0;
        m_nbEpduDiscovered = 0;
        m_nbStsDiscovered  = 0;
        m_nbUpsDiscovered  = 0;
    }

    bool computeIpList(pack::StringList& listIp)
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

    void configureLocalScan()
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

                s = getnameinfo(ifa->ifa_netmask, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0,
                    NI_NUMERICHOST);
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

    bool computeConfigurationFile()
    {
        m_config.clear();
        pack::zconfig::deserializeFile(m_rangeScanConfig.config, m_config);

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

        if (m_config.discovery.scans.empty() &&
            m_config.discovery.type == Config::Discovery::Type::MultiScan) {
            valid = false;
            logError() << "error in config file" << m_rangeScanConfig.config
                       << ": can't have rangescan without range";
        } else if (m_config.discovery.ips.empty() &&
                   m_config.discovery.type == Config::Discovery::Type::IpScan) {
            valid = false;
            logError() << "error in config file" << m_rangeScanConfig.config
                       << ": can't have ipscan without ip list";
        } else {
            if (m_config.discovery.type == Config::Discovery::Type::MultiScan) {
                if (auto sizeTemp = computeScansSize(m_config.discovery.scans)) {
                    m_configurationScan.type      = Config::Discovery::Type::MultiScan;
                    m_configurationScan.scan_size = *sizeTemp;
                    m_configurationScan.scan_list = m_config.discovery.scans.value();
                } else {
                    valid = false;
                    logError() << sizeTemp.error();
                    logError() << "Error in config file" << m_rangeScanConfig.config
                               << ": error in range or subnet";
                }
            } else if (m_config.discovery.type == Config::Discovery::Type::IpScan) {
                if (!computeIpList(m_config.discovery.ips)) {
                    valid = false;
                    logError() << "Error in config file" << m_rangeScanConfig.config << ": error in ip list";
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
            logDbg() << "config file" << m_rangeScanConfig.config << "applied successfully";
        }

        return valid;
    }

    void createAsset(ZMessage&& msg_p)
    {
        if (!msg_p)
            return;

        if (!msg_p.isFtyProto())
            return;

        zmsg_t*      _msg  = msg_p.take();
        fty_proto_t* asset = fty_proto_decode(&_msg);
        const char*  ip    = fty_proto_ext_string(asset, "ip.1", nullptr);
        if (!ip) {
            return;
        }

        bool daisy_chain = fty_proto_ext_string(asset, "daisy_chain", nullptr) != nullptr;
        if (!daisy_chain && m_assets.find("ip", ip)) {
            log_info("Asset with IP address %s already exists", ip);
            return;
        }

        const char* uuid = fty_proto_ext_string(asset, "uuid", nullptr);
        if (uuid && m_assets.find("uuid", uuid)) {
            log_info("Asset with uuid %s already exists", uuid);
            return;
        }

        // set name
        const char* name = fty_proto_ext_string(asset, "hostname", nullptr);
        if (name) {
            fty_proto_ext_insert(asset, "name", "%s", name);
        } else {
            name = fty_proto_aux_string(asset, "subtype", nullptr);
            if (name) {
                fty_proto_ext_insert(asset, "name", "%s (%s)", name, ip);
            } else {
                fty_proto_ext_insert(asset, "name", "%s", ip);
            }
        }

        if (daisy_chain) {
            const char* dc_number = fty_proto_ext_string(asset, "daisy_chain", nullptr);
            name                  = fty_proto_ext_string(asset, "name", nullptr);

            if (streq(dc_number, "1"))
                fty_proto_ext_insert(asset, "name", "%s Host", name);
            else {
                int dc_numberI = atoi(dc_number);
                fty_proto_ext_insert(asset, "name", "%s Device%i", name, dc_numberI - 1);
            }
        }

        std::time_t timestamp = std::time(nullptr);
        char        mbstr[100];
        if (std::strftime(mbstr, sizeof(mbstr), "%FT%T%z", std::localtime(&timestamp))) {
            fty_proto_ext_insert(asset, "create_ts", "%s", mbstr);
        }

        m_devicesDiscovered.mtx_list.lock();
        if (!daisy_chain) {
            char* c = reinterpret_cast<char*>(zhash_first(m_devicesDiscovered.device_list));

            while (c && !streq(c, ip)) {
                c = reinterpret_cast<char*>(zhash_next(m_devicesDiscovered.device_list));
            }

            if (c != nullptr) {
                m_devicesDiscovered.mtx_list.unlock();
                log_info("Asset with IP address %s already exists", ip);
                fty_proto_destroy(&asset);
                return;
            }
        }

        for (const auto& property : m_config.discovery.defaultValuesAux) {
            fty_proto_aux_insert(asset, property.key.value().c_str(), property.value.value().c_str());
        }
        for (const auto& property : m_config.discovery.defaultValuesExt) {
            fty_proto_ext_insert(asset, property.key.value().c_str(), property.value.value().c_str());
        }

        fty_proto_print(asset);
        log_info("Found new asset %s with IP address %s", fty_proto_ext_string(asset, "name", ""), ip);
        fty_proto_set_operation(asset, "create-force");

        fty_proto_t* assetDup = fty_proto_dup(asset);
        zmsg_t*      msg      = fty_proto_encode(&assetDup);
        zmsg_pushstrf(msg, "%s", "READONLY");
        log_debug("about to send create message");
        if (auto rv = m_mlmCreate.sendto("asset-agent", "ASSET_MANIPULATION", nullptr, 10, &msg)) {
            log_info("Create message has been sent to asset-agent (rv = %i)", *rv);

            fty::Expected<zmsg_t*> response = m_mlmCreate.recv();
            if (!response) {
                m_devicesDiscovered.mtx_list.unlock();
                fty_proto_destroy(&asset);
                return;
            }

            char* str_resp = zmsg_popstr(*response);

            if (!str_resp || !streq(str_resp, "OK")) {
                m_devicesDiscovered.mtx_list.unlock();
                log_info("Error during asset creation.");
                fty_proto_destroy(&asset);
                return;
            }

            zstr_free(&str_resp);
            str_resp = zmsg_popstr(*response);
            if (!str_resp) {
                m_devicesDiscovered.mtx_list.unlock();
                log_info("Error during asset creation.");
                fty_proto_destroy(&asset);
                return;
            }

            // create asset links
            for (auto& link : m_defaultValuesLinks) {
                link.dest = uint32_t(std::stoul(strchr(str_resp, '-') + 1));
            }
            auto conn = tntdb::connectCached(DBConn::url);
            DBAssetsInsert::insert_into_asset_links(conn, m_defaultValuesLinks);

            zhash_update(m_devicesDiscovered.device_list, str_resp, strdup(ip));
            zhash_freefn(m_devicesDiscovered.device_list, str_resp, free);
            zstr_free(&str_resp);

            name = fty_proto_aux_string(asset, "subtype", "error");
            if (streq(name, "ups"))
                m_nbUpsDiscovered++;
            else if (streq(name, "epdu"))
                m_nbEpduDiscovered++;
            else if (streq(name, "sts"))
                m_nbStsDiscovered++;
            if (!streq(name, "error"))
                m_nbDiscovered++;
        } else {
            log_error(rv.error().c_str());
        }
        m_devicesDiscovered.mtx_list.unlock();
        fty_proto_destroy(&asset);
    }

    //  --------------------------------------------------------------------------
    //  process pipe message
    //  return true means continue, false means TERM
    //  * $TERM
    //  * BIND
    //  * CONFIG
    //  * SCAN
    //  * LOCALSCAN

    bool handlePipe(ZMessage&& message, zpoller_t* poller)
    {
        if (!message) {
            return true;
        }

        fty::Expected<std::string> command = message.popStr();
        if (!command) {
            logWarn() << command.error();
            return true;
        }

        logDbg() << "s_handle_pipe DO" << *command;

        if (*command == REQ_TERM) {
            logInfo() << "Got $TERM";
            return false;
        } else if (*command == REQ_BIND) {
            auto endpoint = message.popStr();
            auto myname   = message.popStr();
            assert(endpoint && myname);
            m_mlm.connect(*endpoint, 5000, *myname);
            m_mlmCreate.connect(*endpoint, 5000, *myname + ".create");
        } else if (*command == REQ_CONSUMER) {
            auto stream  = message.popStr();
            auto pattern = message.popStr();
            assert(stream && pattern);
            m_mlm.setConsumer(*stream, *pattern);

            // ask for assets now
            ZMessage republish;
            republish.addStr("$all");
            m_mlm.sendto(FTY_ASSET, REQ_REPUBLISH, nullptr, 1000, std::move(republish));
        } else if (*command == REQ_CONFIG) {
            m_rangeScanConfig.config = message.popStr();
            Config conf              = pack::zconfig::deserializeFile<Config>(m_rangeScanConfig.config);

            if (!conf.hasValue()) {
                logError() << "failed to load config file" << m_rangeScanConfig.config;
            }

            bool valid = true;

            if (conf.discovery.scans.empty() && conf.discovery.type == Config::Discovery::Type::MultiScan) {
                valid = false;
                logError() << "error in config file" << m_rangeScanConfig.config
                           << ": can't have rangescan without range";
            } else if (conf.discovery.ips.empty() && conf.discovery.type == Config::Discovery::Type::IpScan) {
                valid = false;
                logError() << "error in config file " << m_rangeScanConfig.config
                           << ": can't have ipscan without ip list";
            } else {
                auto sizeTemp = computeScansSize(conf.discovery.scans);
                if (sizeTemp && computeIpList(conf.discovery.ips)) {
                    // ok, validate the config
                    m_configurationScan.type      = conf.discovery.type;
                    m_configurationScan.scan_size = *sizeTemp;
                    m_configurationScan.scan_list.clear();
                    m_configurationScan.scan_list = conf.discovery.scans.value();

                    if (m_configurationScan.type == Config::Discovery::Type::IpScan) {
                        m_configurationScan.scan_size = conf.discovery.ips.size();
                        m_configurationScan.scan_list.clear();
                        m_configurationScan.scan_list = conf.discovery.ips.value();
                    }
                } else {
                    valid = false;
                    logError() << "error in config file" << m_rangeScanConfig.config << ": error in scans";
                }
            }

            std::string mappingPath = conf.parameters.mappingFile;
            if (mappingPath == "none") {
                logError() << "No mapping file declared under config key '"
                           << conf.parameters.mappingFile.key() << "'";
                valid = false;
            } else {
                try {
                    m_nutMappingInventory = fty::nut::loadMapping(mappingPath, "inventoryMapping");
                    logInfo() << "Mapping file '" << mappingPath << "' loaded,"
                              << m_nutMappingInventory.size() << "inventory mappings";
                } catch (const std::exception& e) {
                    logError() << "Couldn't load mapping file '" << mappingPath << "': " << e.what();
                    valid = false;
                }
            }

            if (valid) {
                logDbg() << "config file" << m_rangeScanConfig.config << "applied successfully";
            }
        } else if (*command == REQ_SCAN) {
            if (m_rangeScanner) {
                reset();
                zpoller_remove(poller, m_rangeScanner);
                zactor_destroy(&m_rangeScanner);
            }
            m_ongoingStop = false;
            m_localScanSubScan.clear();
            m_scanSize = 0;

            m_rangeScanConfig.ranges.emplace_back(*message.popStr(), nullptr);
            if (!m_rangeScanConfig.ranges[0].first.empty()) {
                CIDRAddress addrCIDR((m_rangeScanConfig.ranges[0]).first);

                if (addrCIDR.prefix() != -1) {
                    // all the subnet (1 << (32- prefix) ) minus subnet and broadcast address
                    if (addrCIDR.prefix() <= 30)
                        m_scanSize = ((1 << (32 - addrCIDR.prefix())) - 2);
                    else // 31/32 prefix special management
                        m_scanSize = (1 << (32 - addrCIDR.prefix()));
                } else {
                    // not a valid range
                    logError() << "Address range (" << m_rangeScanConfig.ranges[0].first << ") is not valid!";
                }
            }
        } else if (*command == REQ_LOCALSCAN) {
            if (m_rangeScanner) {
                reset();
                zpoller_remove(poller, m_rangeScanner);
                zactor_destroy(&m_rangeScanner);
            }

            m_ongoingStop = false;
            m_localScanSubScan.clear();
            m_scanSize = 0;
            configureLocalScan();

            m_rangeScanConfig.ranges.clear();
            if (m_scanSize > 0) {
                while (m_localScanSubScan.size() > 0) {
                    zmsg_t* zmfalse = zmsg_new();
                    zmsg_addstr(zmfalse, m_localScanSubScan.back().c_str());

                    char* secondnullptr = nullptr;
                    m_rangeScanConfig.ranges.push_back(std::make_pair(zmsg_popstr(zmfalse), secondnullptr));
                    m_localScanSubScan.pop_back();
                    zmsg_destroy(&zmfalse);
                }

                reset();
            }

        } else {
            logError() << "s_handle_pipe: Unknown command: " << *command << ".\n";
        }
        return true;
    }

    //  --------------------------------------------------------------------------
    //  process message from MAILBOX DELIVER
    //  * SETCONFIG,
    //       REQ : <uuid> <type_of_scan><nb_of_scan><scan1><scan2>..
    //  * GETCONFIG
    //       REQ : <uuid>
    //  * LAUNCHSCAN
    //       REQ : <uuid>
    //  * PROGRESS
    //       REQ : <uuid>
    //  * STOPSCAN
    //       REQ : <uuid>

    void handleMailbox(ZMessage&& msg, zpoller_t* poller)
    {
        if (msg.isFtyProto()) {
            FtyProto proto = FtyProto::decode(std::move(msg));
            m_assets.put(std::move(proto));
        } else {
            // handle REST API requests
            auto cmd = msg.popStr();
            if (!cmd) {
                logWarn() << "s_handle_mailbox" << cmd.error();
                return;
            }

            logDbg() << "s_handle_mailbox DO :" << *cmd;

            if (*cmd == REQ_LAUNCHSCAN) {
                // LAUNCHSCAN
                // REQ <uuid>
                auto     zuuid = msg.popStr();
                ZMessage reply;
                reply.addStr(*zuuid);

                if (m_rangeScanner) {
                    if (m_ongoingStop)
                        reply.addStr(STATUS_STOPPING);
                    else
                        reply.addStr(STATUS_RUNNING);
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
                                    m_rangeScanConfig.ranges.push_back(
                                        std::make_pair(m_localScanSubScan.back(), nullptr));

                                    logDbg() << "Range scanner requested for"
                                             << m_rangeScanConfig.ranges.back().first << "with config file"
                                             << m_rangeScanConfig.config;
                                    m_localScanSubScan.pop_back();
                                }
                                // create range scanner
                                if (m_rangeScanner) {
                                    zpoller_remove(poller, m_rangeScanner);
                                    zactor_destroy(&m_rangeScanner);
                                }
                                reset();
                                m_rangeScanner = rangeScannerNew();
                                zpoller_add(poller, m_rangeScanner);
                                m_statusScan = STATUS_PROGESS;

                                reply.addStr(RESP_OK);
                            } else {
                                reply.addStr(RESP_ERR);
                            }

                        } else if ((m_configurationScan.type == Config::Discovery::Type::MultiScan) ||
                                   (m_configurationScan.type == Config::Discovery::Type::IpScan)) {
                            // Launch rangeScan
                            m_localScanSubScan = m_configurationScan.scan_list;
                            m_scanSize         = m_configurationScan.scan_size;

                            while (m_localScanSubScan.size() > 0) {
                                std::string next_scan = m_localScanSubScan.back();
                                auto [first, last]    = fty::split<std::string, std::string>(next_scan, "-");
                                m_rangeScanConfig.ranges.emplace_back(first, last);

                                logDbg() << "Range scanner requested for" << next_scan << "with config file"
                                         << m_rangeScanConfig.config;

                                m_localScanSubScan.pop_back();
                            }
                            // create range scanner
                            if (m_rangeScanner) {
                                zpoller_remove(poller, m_rangeScanner);
                                zactor_destroy(&m_rangeScanner);
                            }
                            reset();
                            m_rangeScanner = rangeScannerNew();
                            zpoller_add(poller, m_rangeScanner);

                            m_statusScan = STATUS_PROGESS;
                            reply.addStr(RESP_OK);
                        } else {
                            reply.addStr(RESP_ERR);
                        }
                    } else {
                        reply.addStr(RESP_ERR);
                    }
                }
                m_mlm.sendto(m_mlm.sender(), m_mlm.subject(), m_mlm.tracker(), 1000, std::move(reply));
            } else if (*cmd == REQ_PROGRESS) {
                // PROGRESS
                // REQ <uuid>
                auto     zuuid = msg.popStr();
                ZMessage reply;
                reply.addStr(*zuuid);
                if (!m_percent.empty()) {
                    reply.addStr(RESP_OK);
                    reply.addStr(fty::convert<std::string>(m_statusScan) + PRIi32);
                    reply.addStr(m_percent);
                    reply.addStr(fty::convert<std::string>(m_nbDiscovered) + PRIi64);
                    reply.addStr(fty::convert<std::string>(m_nbUpsDiscovered) + PRIi64);
                    reply.addStr(fty::convert<std::string>(m_nbEpduDiscovered) + PRIi64);
                    reply.addStr(fty::convert<std::string>(m_nbStsDiscovered) + PRIi64);
                } else {
                    reply.addStr(RESP_OK);
                    reply.addStr("-1" PRIi32);
                }
                m_mlm.sendto(m_mlm.sender(), m_mlm.subject(), m_mlm.tracker(), 1000, std::move(reply));
            } else if (*cmd == REQ_STOPSCAN) {
                // STOPSCAN
                // REQ <uuid>
                auto     zuuid = msg.popStr();
                ZMessage reply;
                reply.addStr(*zuuid);
                reply.addStr(RESP_OK);

                m_mlm.sendto(m_mlm.sender(), m_mlm.subject(), m_mlm.tracker(), 1000, std::move(reply));
                if (m_rangeScanner && !m_ongoingStop) {
                    m_statusScan  = STATUS_STOPPED;
                    m_ongoingStop = true;
                    zstr_send(m_rangeScanner, REQ_TERM);
                }

                m_localScanSubScan.clear();
                m_scanSize = 0;
            } else {
                logError() << "s_handle_mailbox: Unknown actor command:" << *cmd << ".\n";
            }
        }
    }

    //  --------------------------------------------------------------------------
    //  process message stream

    void handleStream(ZMessage&& message)
    {
        if (message.isFtyProto()) {
            // handle fty_proto protocol here
            zmsg_t*      msg  = message.take();
            fty_proto_t* fmsg = fty_proto_decode(&msg);

            if (fmsg && (fty_proto_id(fmsg) == FTY_PROTO_ASSET)) {

                const char* operation = fty_proto_operation(fmsg);

                // TODO : Remove as soon as we can this ugly hack of "_no_not_really"
                if (streq(operation, FTY_PROTO_ASSET_OP_DELETE) &&
                    !zhash_lookup(fty_proto_aux(fmsg), "_no_not_really")) {
                    const char* iname = fty_proto_name(fmsg);
                    m_devicesDiscovered.mtx_list.lock();
                    zhash_delete(m_devicesDiscovered.device_list, iname);
                    m_devicesDiscovered.mtx_list.unlock();
                } else if (streq(operation, FTY_PROTO_ASSET_OP_CREATE) ||
                           streq(operation, FTY_PROTO_ASSET_OP_UPDATE)) {
                    const char* iname = fty_proto_name(fmsg);
                    const char* ip    = fty_proto_ext_string(fmsg, "ip.1", "");
                    if (!streq(ip, "")) {
                        m_devicesDiscovered.mtx_list.lock();
                        zhash_update(m_devicesDiscovered.device_list, iname, strdup(ip));
                        zhash_freefn(m_devicesDiscovered.device_list, iname, free);
                        m_devicesDiscovered.mtx_list.unlock();
                    }
                }
            }

            fty_proto_destroy(&fmsg);
        }
    }

    //  --------------------------------------------------------------------------
    //  process message stream
    //  * DONE
    //  * FOUND
    //  * POOGRESS

    void handleRangeScanner(ZMessage&& msg, zpoller_t* poller, zsock_t* pipe)
    {
        msg.print();
        fty::Expected<std::string> cmd = msg.popStr();
        if (!cmd) {
            logWarn() << "s_handle_range_scanner" << cmd.error();
            return;
        }

        logDbg() << "s_handle_range_scanner DO :" << *cmd;
        if (*cmd == REQ_DONE) {
            if (m_ongoingStop && m_rangeScanner) {
                zpoller_remove(poller, m_rangeScanner);
                zactor_destroy(&m_rangeScanner);
            } else {
                zstr_send(pipe, REQ_DONE);
                if (!m_localScanSubScan.empty())
                    m_localScanSubScan.clear();

                m_statusScan = STATUS_FINISHED;
                m_rangeScanConfig.ranges.clear();

                zpoller_remove(poller, m_rangeScanner);
                zactor_destroy(&m_rangeScanner);
            }
        } else if (*cmd == REQ_FOUND) {
            createAsset(std::move(msg));
        } else if (*cmd == REQ_PROGRESS) {
            m_nbPercent++;

            m_percent = std::to_string(m_nbPercent * 100 / m_scanSize);
            // other cmd. NOTFOUND must not be treated
        } else if (*cmd != "NOTFOUND") {
            logError() << "s_handle_range_scanner: Unknown  command:" << *cmd << ".\n";
        }
    }

    zactor_t* rangeScannerNew()
    {
        zlist_t* args = zlist_new();
        zlist_append(args, &m_rangeScanConfig);
        zlist_append(args, &m_devicesDiscovered);
        zlist_append(args, &m_nutMappingInventory);
        return zactor_new(range_scan_actor, args);
    }

private:
    Mlm                      m_mlm;
    Mlm                      m_mlmCreate;
    Assets                   m_assets;
    int64_t                  m_nbPercent;
    int64_t                  m_nbDiscovered;
    int64_t                  m_scanSize;
    int64_t                  m_nbUpsDiscovered;
    int64_t                  m_nbEpduDiscovered;
    int64_t                  m_nbStsDiscovered;
    int32_t                  m_statusScan;
    bool                     m_ongoingStop;
    std::vector<std::string> m_localScanSubScan;
    range_scan_args_t        m_rangeScanConfig;
    configuration_scan_t     m_configurationScan;
    zactor_t*                m_rangeScanner;
    std::string              m_percent;
    discovered_devices_t     m_devicesDiscovered;
    fty::nut::KeyValues      m_nutMappingInventory;
    std::vector<link_t>      m_defaultValuesLinks;
    Config                   m_config;
};

Discovery::Discovery()
    : Actor(std::unique_ptr<Impl>(new Impl))
{
}

Discovery::~Discovery()
{
}

bool Discovery::init()
{
    return m_impl->init();
}

std::string toString(Discovery::Command cmd)
{
    switch (cmd) {
        case Discovery::Command::Bind:
            return "BIND";
        case Discovery::Command::Done:
            return "DONE";
        case Discovery::Command::Scan:
            return "SCAN";
        case Discovery::Command::Term:
            return "$TERM";
        case Discovery::Command::Found:
            return "FOUND";
        case Discovery::Command::Config:
            return "CONFIG";
        case Discovery::Command::Consumer:
            return "CONSUMER";
        case Discovery::Command::Progress:
            return "PROGRESS";
        case Discovery::Command::StopScan:
            return "STOPSCAN";
        case Discovery::Command::GetConfig:
            return "GETCONFIG";
        case Discovery::Command::LocalScan:
            return "LOCALSCAN";
        case Discovery::Command::Republish:
            return "REPUBLISH";
        case Discovery::Command::SetConfig:
            return "SETCONFIG";
        case Discovery::Command::LaunchScan:
            return "LAUNCHSCAN";
    }
}

Discovery::Command fromString(const std::string& str)
{
    if (str == "BIND") {
        return Discovery::Command::Bind;
    } else if (str == "DONE") {
        return Discovery::Command::Done;
    } else if (str == "SCAN") {
        return Discovery::Command::Scan;
    } else if (str == "$TERM") {
        return Discovery::Command::Term;
    } else if (str == "FOUND") {
        return Discovery::Command::Found;
    } else if (str == "CONFIG") {
        return Discovery::Command::Config;
    } else if (str == "CONSUMER") {
        return Discovery::Command::Consumer;
    } else if (str == "PROGRESS") {
        return Discovery::Command::Progress;
    } else if (str == "STOPSCAN") {
        return Discovery::Command::StopScan;
    } else if (str == "GETCONFIG") {
        return Discovery::Command::GetConfig;
    } else if (str == "LOCALSCAN") {
        return Discovery::Command::LocalScan;
    } else if (str == "REPUBLISH") {
        return Discovery::Command::Republish;
    } else if (str == "SETCONFIG") {
        return Discovery::Command::SetConfig;
    } else if (str == "LAUNCHSCAN") {
        return Discovery::Command::LaunchScan;
    }
    throw std::runtime_error(str + " is wrong command");
}
