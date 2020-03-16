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
#include <fty/fty-log.h>
#include <fty/split.h>
#include <fty/expected.h>
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

struct _fty_discovery_server_t
{
    Mlm                                mlm;
    Mlm                                mlmCreate;
    zactor_t*                          scanner;
    assets_t*                          assets;
    int64_t                            nb_percent;
    int64_t                            nb_discovered;
    int64_t                            scan_size;
    int64_t                            nb_ups_discovered;
    int64_t                            nb_epdu_discovered;
    int64_t                            nb_sts_discovered;
    int32_t                            status_scan;
    bool                               ongoing_stop;
    std::vector<std::string>           localscan_subscan;
    range_scan_args_t                  range_scan_config;
    configuration_scan_t               configuration_scan;
    zactor_t*                          range_scanner;
    std::string                        percent;
    discovered_devices_t               devices_discovered;
    fty::nut::KeyValues                nut_mapping_inventory;
    std::map<std::string, std::string> default_values_aux;
    std::map<std::string, std::string> default_values_ext;
    std::vector<link_t>                default_values_links;
};

zactor_t* range_scanner_new(fty_discovery_server_t* self)
{
    zlist_t* args = zlist_new();
    zlist_append(args, &self->range_scan_config);
    zlist_append(args, &self->devices_discovered);
    zlist_append(args, &self->nut_mapping_inventory);
    return zactor_new(range_scan_actor, args);
}

void reset_nb_discovered(fty_discovery_server_t* self)
{
    self->nb_discovered      = 0;
    self->nb_epdu_discovered = 0;
    self->nb_sts_discovered  = 0;
    self->nb_ups_discovered  = 0;
}

bool compute_ip_list(pack::StringList& listIp)
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

fty::Expected<int64_t> compute_scans_size(pack::StringList& list_scan)
{
    int64_t scan_size = 0;
    for (std::string& scan : list_scan) {
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

void configure_local_scan(fty_discovery_server_t* self)
{
    int             s, family, prefix;
    char            host[NI_MAXHOST];
    struct ifaddrs *ifaddr, *ifa;
    std::string     addr, netm, addrmask;

    self->scan_size = 0;
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
                self->scan_size += ((1 << (32 - prefix)) - 2);
            else // 31/32 prefix special management
                self->scan_size += (1 << (32 - prefix));


            CIDRAddress addrCidr(addr, uint(prefix));

            addrmask.clear();
            addrmask.assign(addrCidr.network().toString());

            self->localscan_subscan.push_back(addrmask);
            logInfo() << "Localscan subnet found for" << ifa->ifa_name << ":" << addrmask;
        }

        freeifaddrs(ifaddr);
    }
}

bool compute_configuration_file(fty_discovery_server_t* self)
{
    Config conf = pack::zconfig::deserializeFile<Config>(self->range_scan_config.config);

    self->default_values_aux = conf.discovery.defaultValuesAux.value();
    self->default_values_ext = conf.discovery.defaultValuesExt.value();

    self->default_values_links.clear();
    for (const auto& val : conf.discovery.defaultValuesLinks) {
        link_t l;
        l.src     = val.src;
        l.dest    = 0;
        l.src_out = nullptr;
        l.dest_in = nullptr;
        l.type    = uint16_t(val.type);

        if (l.src > 0) {
            self->default_values_links.emplace_back(l);
        }
    }

    bool valid = true;

    if (conf.discovery.scans.empty() && conf.discovery.type == Config::Discovery::Type::multiscan) {
        valid = false;
        logError() << "error in config file" << self->range_scan_config.config
                   << ": can't have rangescan without range";
    } else if (conf.discovery.ips.empty() && conf.discovery.type == Config::Discovery::Type::ipscan) {
        valid = false;
        logError() << "error in config file" << self->range_scan_config.config
                   << ": can't have ipscan without ip list";
    } else {
        if (conf.discovery.type == Config::Discovery::Type::multiscan) {
            if (auto sizeTemp = compute_scans_size(conf.discovery.scans)) {
                self->configuration_scan.type      = Config::Discovery::Type::multiscan;
                self->configuration_scan.scan_size = *sizeTemp;
                self->configuration_scan.scan_list = conf.discovery.scans.value();
            } else {
                valid = false;
                logError() << sizeTemp.error();
                logError() << "Error in config file" << self->range_scan_config.config
                           << ": error in range or subnet";
            }
        } else if (conf.discovery.type == Config::Discovery::Type::ipscan) {
            if (!compute_ip_list(conf.discovery.ips)) {
                valid = false;
                logError() << "Error in config file" << self->range_scan_config.config
                           << ": error in ip list";
            } else {
                self->configuration_scan.type      = Config::Discovery::Type::ipscan;
                self->configuration_scan.scan_size = int64_t(conf.discovery.ips.size());
                self->configuration_scan.scan_list = conf.discovery.ips.value();
            }
        } else if (conf.discovery.type == Config::Discovery::Type::localscan) {
            self->configuration_scan.type = Config::Discovery::Type::localscan;
        } else {
            valid = false;
        }
    }

    if (valid) {
        logDbg() << "config file" << self->range_scan_config.config << "applied successfully";
    }

    return valid;
}


//  --------------------------------------------------------------------------
//  send create asset if it is new

void ftydiscovery_create_asset(fty_discovery_server_t* self, ZMessage&& msg_p)
{
    if (!self || !msg_p)
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
    if (!daisy_chain && assets_find(self->assets, "ip", ip)) {
        log_info("Asset with IP address %s already exists", ip);
        return;
    }

    const char* uuid = fty_proto_ext_string(asset, "uuid", nullptr);
    if (uuid && assets_find(self->assets, "uuid", uuid)) {
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

    self->devices_discovered.mtx_list.lock();
    if (!daisy_chain) {
        char* c = reinterpret_cast<char*>(zhash_first(self->devices_discovered.device_list));

        while (c && !streq(c, ip)) {
            c = reinterpret_cast<char*>(zhash_next(self->devices_discovered.device_list));
        }

        if (c != nullptr) {
            self->devices_discovered.mtx_list.unlock();
            log_info("Asset with IP address %s already exists", ip);
            fty_proto_destroy(&asset);
            return;
        }
    }

    for (const auto& property : self->default_values_aux) {
        fty_proto_aux_insert(asset, property.first.c_str(), property.second.c_str());
    }
    for (const auto& property : self->default_values_ext) {
        fty_proto_ext_insert(asset, property.first.c_str(), property.second.c_str());
    }

    fty_proto_print(asset);
    log_info("Found new asset %s with IP address %s", fty_proto_ext_string(asset, "name", ""), ip);
    fty_proto_set_operation(asset, "create-force");

    fty_proto_t* assetDup = fty_proto_dup(asset);
    zmsg_t*      msg      = fty_proto_encode(&assetDup);
    zmsg_pushstrf(msg, "%s", "READONLY");
    log_debug("about to send create message");
    if (auto rv = self->mlmCreate.sendto("asset-agent", "ASSET_MANIPULATION", nullptr, 10, &msg)) {
        log_info("Create message has been sent to asset-agent (rv = %i)", *rv);

        fty::Expected<zmsg_t*> response = self->mlmCreate.recv();
        if (!response) {
            self->devices_discovered.mtx_list.unlock();
            fty_proto_destroy(&asset);
            return;
        }

        char* str_resp = zmsg_popstr(*response);

        if (!str_resp || !streq(str_resp, "OK")) {
            self->devices_discovered.mtx_list.unlock();
            log_info("Error during asset creation.");
            fty_proto_destroy(&asset);
            return;
        }

        zstr_free(&str_resp);
        str_resp = zmsg_popstr(*response);
        if (!str_resp) {
            self->devices_discovered.mtx_list.unlock();
            log_info("Error during asset creation.");
            fty_proto_destroy(&asset);
            return;
        }

        // create asset links
        for (auto& link : self->default_values_links) {
            link.dest = uint32_t(std::stoul(strchr(str_resp, '-') + 1));
        }
        auto conn = tntdb::connectCached(DBConn::url);
        DBAssetsInsert::insert_into_asset_links(conn, self->default_values_links);

        zhash_update(self->devices_discovered.device_list, str_resp, strdup(ip));
        zhash_freefn(self->devices_discovered.device_list, str_resp, free);
        zstr_free(&str_resp);

        name = fty_proto_aux_string(asset, "subtype", "error");
        if (streq(name, "ups"))
            self->nb_ups_discovered++;
        else if (streq(name, "epdu"))
            self->nb_epdu_discovered++;
        else if (streq(name, "sts"))
            self->nb_sts_discovered++;
        if (!streq(name, "error"))
            self->nb_discovered++;
    } else {
        log_error(rv.error().c_str());
    }
    self->devices_discovered.mtx_list.unlock();
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

static bool s_handle_pipe(fty_discovery_server_t* self, ZMessage&& message, zpoller_t* poller)
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
        self->mlm.connect(*endpoint, 5000, *myname);
        self->mlmCreate.connect(*endpoint, 5000, *myname + ".create");
    } else if (*command == REQ_CONSUMER) {
        auto stream  = message.popStr();
        auto pattern = message.popStr();
        assert(stream && pattern);
        self->mlm.setConsumer(*stream, *pattern);

        // ask for assets now
        ZMessage republish;
        republish.addStr("$all");
        self->mlm.sendto(FTY_ASSET, REQ_REPUBLISH, nullptr, 1000, std::move(republish));
    } else if (*command == REQ_CONFIG) {
        self->range_scan_config.config = message.popStr();
        Config conf = pack::zconfig::deserializeFile<Config>(self->range_scan_config.config);

        if (!conf.hasValue()) {
            logError() << "failed to load config file" << self->range_scan_config.config;
        }

        bool valid = true;

        if (conf.discovery.scans.empty() && conf.discovery.type == Config::Discovery::Type::multiscan) {
            valid = false;
            logError() << "error in config file" << self->range_scan_config.config
                       << ": can't have rangescan without range";
        } else if (conf.discovery.ips.empty() && conf.discovery.type == Config::Discovery::Type::ipscan) {
            valid = false;
            logError() << "error in config file " << self->range_scan_config.config
                       << ": can't have ipscan without ip list";
        } else {
            auto sizeTemp = compute_scans_size(conf.discovery.scans);
            if (sizeTemp && compute_ip_list(conf.discovery.ips)) {
                // ok, validate the config
                self->configuration_scan.type      = conf.discovery.type;
                self->configuration_scan.scan_size = *sizeTemp;
                self->configuration_scan.scan_list.clear();
                self->configuration_scan.scan_list = conf.discovery.scans.value();

                if (self->configuration_scan.type == Config::Discovery::Type::ipscan) {
                    self->configuration_scan.scan_size = conf.discovery.ips.size();
                    self->configuration_scan.scan_list.clear();
                    self->configuration_scan.scan_list = conf.discovery.ips.value();
                }
            } else {
                valid = false;
                logError() << "error in config file" << self->range_scan_config.config << ": error in scans";
            }
        }

        std::string mappingPath = conf.parameters.mappingFile;
        if (mappingPath == "none") {
            logError() << "No mapping file declared under config key '" << conf.parameters.mappingFile.key()
                       << "'";
            valid = false;
        } else {
            try {
                self->nut_mapping_inventory = fty::nut::loadMapping(mappingPath, "inventoryMapping");
                logInfo() << "Mapping file '" << mappingPath << "' loaded,"
                          << self->nut_mapping_inventory.size() << "inventory mappings";
            } catch (const std::exception& e) {
                logError() << "Couldn't load mapping file '" << mappingPath << "': " << e.what();
                valid = false;
            }
        }

        if (valid) {
            logDbg() << "config file" << self->range_scan_config.config << "applied successfully";
        }
    } else if (*command == REQ_SCAN) {
        if (self->range_scanner) {
            reset_nb_discovered(self);
            zpoller_remove(poller, self->range_scanner);
            zactor_destroy(&self->range_scanner);
        }
        self->ongoing_stop = false;
        self->localscan_subscan.clear();
        self->scan_size = 0;

        self->range_scan_config.ranges.emplace_back(*message.popStr(), nullptr);
        if (!self->range_scan_config.ranges[0].first.empty()) {
            CIDRAddress addrCIDR((self->range_scan_config.ranges[0]).first);

            if (addrCIDR.prefix() != -1) {
                // all the subnet (1 << (32- prefix) ) minus subnet and broadcast address
                if (addrCIDR.prefix() <= 30)
                    self->scan_size = ((1 << (32 - addrCIDR.prefix())) - 2);
                else // 31/32 prefix special management
                    self->scan_size = (1 << (32 - addrCIDR.prefix()));
            } else {
                // not a valid range
                logError() << "Address range (" << self->range_scan_config.ranges[0].first
                           << ") is not valid!";
            }
        }
    } else if (*command == REQ_LOCALSCAN) {
        if (self->range_scanner) {
            reset_nb_discovered(self);
            zpoller_remove(poller, self->range_scanner);
            zactor_destroy(&self->range_scanner);
        }

        self->ongoing_stop = false;
        self->localscan_subscan.clear();
        self->scan_size = 0;
        configure_local_scan(self);

        self->range_scan_config.ranges.clear();
        if (self->scan_size > 0) {
            while (self->localscan_subscan.size() > 0) {
                zmsg_t* zmfalse = zmsg_new();
                zmsg_addstr(zmfalse, self->localscan_subscan.back().c_str());

                char* secondnullptr = nullptr;
                self->range_scan_config.ranges.push_back(std::make_pair(zmsg_popstr(zmfalse), secondnullptr));
                self->localscan_subscan.pop_back();
                zmsg_destroy(&zmfalse);
            }

            reset_nb_discovered(self);
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

void static s_handle_mailbox(fty_discovery_server_t* self, ZMessage&& msg, zpoller_t* poller)
{
    if (msg.isFtyProto()) {
        zmsg_t*      pass = msg.take();
        fty_proto_t* fmsg = fty_proto_decode(&pass);
        assets_put(self->assets, &fmsg);
        fty_proto_destroy(&fmsg);
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

            if (self->range_scanner) {
                if (self->ongoing_stop)
                    reply.addStr(STATUS_STOPPING);
                else
                    reply.addStr(STATUS_RUNNING);
            } else {
                self->ongoing_stop = false;
                self->percent      = "0";

                self->localscan_subscan.clear();
                self->scan_size  = 0;
                self->nb_percent = 0;

                if (compute_configuration_file(self)) {
                    if (self->configuration_scan.type == Config::Discovery::Type::localscan) {
                        // Launch localScan
                        configure_local_scan(self);

                        if (self->scan_size > 0) {
                            while (self->localscan_subscan.size() > 0) {
                                self->range_scan_config.ranges.push_back(
                                    std::make_pair(self->localscan_subscan.back(), nullptr));

                                logDbg() << "Range scanner requested for"
                                         << self->range_scan_config.ranges.back().first << "with config file"
                                         << self->range_scan_config.config;
                                self->localscan_subscan.pop_back();
                            }
                            // create range scanner
                            if (self->range_scanner) {
                                zpoller_remove(poller, self->range_scanner);
                                zactor_destroy(&self->range_scanner);
                            }
                            reset_nb_discovered(self);
                            self->range_scanner = range_scanner_new(self);
                            zpoller_add(poller, self->range_scanner);
                            self->status_scan = STATUS_PROGESS;

                            reply.addStr(RESP_OK);
                        } else {
                            reply.addStr(RESP_ERR);
                        }

                    } else if ((self->configuration_scan.type == Config::Discovery::Type::multiscan) ||
                               (self->configuration_scan.type == Config::Discovery::Type::ipscan)) {
                        // Launch rangeScan
                        self->localscan_subscan = self->configuration_scan.scan_list;
                        self->scan_size         = self->configuration_scan.scan_size;

                        while (self->localscan_subscan.size() > 0) {
                            std::string next_scan = self->localscan_subscan.back();
                            auto [first, last]    = fty::split<std::string, std::string>(next_scan, "-");
                            self->range_scan_config.ranges.emplace_back(first, last);

                            logDbg() << "Range scanner requested for" << next_scan << "with config file"
                                     << self->range_scan_config.config;

                            self->localscan_subscan.pop_back();
                        }
                        // create range scanner
                        if (self->range_scanner) {
                            zpoller_remove(poller, self->range_scanner);
                            zactor_destroy(&self->range_scanner);
                        }
                        reset_nb_discovered(self);
                        self->range_scanner = range_scanner_new(self);
                        zpoller_add(poller, self->range_scanner);

                        self->status_scan = STATUS_PROGESS;
                        reply.addStr(RESP_OK);
                    } else {
                        reply.addStr(RESP_ERR);
                    }
                } else {
                    reply.addStr(RESP_ERR);
                }
            }
            self->mlm.sendto(
                self->mlm.sender(), self->mlm.subject(), self->mlm.tracker(), 1000, std::move(reply));
        } else if (*cmd == REQ_PROGRESS) {
            // PROGRESS
            // REQ <uuid>
            auto     zuuid = msg.popStr();
            ZMessage reply;
            reply.addStr(*zuuid);
            if (!self->percent.empty()) {
                reply.addStr(RESP_OK);
                reply.addStr(fty::convert<std::string>(self->status_scan) + PRIi32);
                reply.addStr(self->percent);
                reply.addStr(fty::convert<std::string>(self->nb_discovered) + PRIi64);
                reply.addStr(fty::convert<std::string>(self->nb_ups_discovered) + PRIi64);
                reply.addStr(fty::convert<std::string>(self->nb_epdu_discovered) + PRIi64);
                reply.addStr(fty::convert<std::string>(self->nb_sts_discovered) + PRIi64);
            } else {
                reply.addStr(RESP_OK);
                reply.addStr("-1" PRIi32);
            }
            self->mlm.sendto(
                self->mlm.sender(), self->mlm.subject(), self->mlm.tracker(), 1000, std::move(reply));
        } else if (*cmd == REQ_STOPSCAN) {
            // STOPSCAN
            // REQ <uuid>
            auto     zuuid = msg.popStr();
            ZMessage reply;
            reply.addStr(*zuuid);
            reply.addStr(RESP_OK);

            self->mlm.sendto(
                self->mlm.sender(), self->mlm.subject(), self->mlm.tracker(), 1000, std::move(reply));
            if (self->range_scanner && !self->ongoing_stop) {
                self->status_scan  = STATUS_STOPPED;
                self->ongoing_stop = true;
                zstr_send(self->range_scanner, REQ_TERM);
            }

            self->localscan_subscan.clear();
            self->scan_size = 0;
        } else {
            logError() << "s_handle_mailbox: Unknown actor command:" << *cmd << ".\n";
        }
    }
}

//  --------------------------------------------------------------------------
//  process message stream

void static s_handle_stream(fty_discovery_server_t* self, ZMessage&& message)
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
                self->devices_discovered.mtx_list.lock();
                zhash_delete(self->devices_discovered.device_list, iname);
                self->devices_discovered.mtx_list.unlock();
            } else if (streq(operation, FTY_PROTO_ASSET_OP_CREATE) ||
                       streq(operation, FTY_PROTO_ASSET_OP_UPDATE)) {
                const char* iname = fty_proto_name(fmsg);
                const char* ip    = fty_proto_ext_string(fmsg, "ip.1", "");
                if (!streq(ip, "")) {
                    self->devices_discovered.mtx_list.lock();
                    zhash_update(self->devices_discovered.device_list, iname, strdup(ip));
                    zhash_freefn(self->devices_discovered.device_list, iname, free);
                    self->devices_discovered.mtx_list.unlock();
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

void static s_handle_range_scanner(
    fty_discovery_server_t* self, ZMessage&& msg, zpoller_t* poller, zsock_t* pipe)
{
    msg.print();
    fty::Expected<std::string> cmd = msg.popStr();
    if (!cmd) {
        logWarn() << "s_handle_range_scanner" << cmd.error();
        return;
    }

    logDbg() << "s_handle_range_scanner DO :" << *cmd;
    if (*cmd == REQ_DONE) {
        if (self->ongoing_stop && self->range_scanner) {
            zpoller_remove(poller, self->range_scanner);
            zactor_destroy(&self->range_scanner);
        } else {
            zstr_send(pipe, REQ_DONE);
            if (!self->localscan_subscan.empty())
                self->localscan_subscan.clear();

            self->status_scan = STATUS_FINISHED;
            self->range_scan_config.ranges.clear();

            zpoller_remove(poller, self->range_scanner);
            zactor_destroy(&self->range_scanner);
        }
    } else if (*cmd == REQ_FOUND) {
        ftydiscovery_create_asset(self, std::move(msg));
    } else if (*cmd == REQ_PROGRESS) {
        self->nb_percent++;

        self->percent = std::to_string(self->nb_percent * 100 / self->scan_size);
        // other cmd. NOTFOUND must not be treated
    } else if (*cmd != "NOTFOUND") {
        logError() << "s_handle_range_scanner: Unknown  command:" << *cmd << ".\n";
    }
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

class Discovery::Impl
{
private:
};

Discovery::Discovery():
    m_impl(new Impl)
{
}

Discovery::~Discovery()
{
}


