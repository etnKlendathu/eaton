/*  =========================================================================
    fty_discovery - Agent performing device discovery in network

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
    fty_discovery - Agent performing device discovery in network
@discuss
@end
*/

#include "fty_discovery_classes.h"
#include <fty/command-line.h>
#include <fty/fty-log.h>


int main(int argc, char* argv[])
{
    bool        verbose = false;
    bool        agent   = false;
    bool        help    = false;
    std::string range;
    std::string config = FTY_DISCOVERY_CFG_FILE;

    // clang-format off
    fty::CommandLine cmd(
        "Agent performing device discovery in network",
        {
            {"--verbose|-v", verbose, "verbose output"},
            {"--agent|-a",   agent,   "stay running, listen to rest api commands"},
            {"--config|-c",  config,  "agent config file"},
            {"--range|-r",   range,   "scan this range (192.168.1.0/24 format). If -a and -r are not"
                                      "present, scan of attached networks is performed (localscan)"},
            {"--help|-h",    help,    "display this help"}
        }
    );
    // clang-format on

    try {
        cmd.parse(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        std::cout << cmd.help() << std::endl;
        return 1;
    }

    if (help) {
        std::cout << cmd.help() << std::endl;
        return 0;
    }

    Config conf = pack::zconfig::deserializeFile<Config>(config);

    fty::ManageFtyLog::getInstanceFtylog().setConfigFile(conf.log.config);

    if (verbose) {
        fty::ManageFtyLog::getInstanceFtylog().setVeboseMode();
    }

    zsys_init();
    DBConn::dbpath();
    logDbg() << "fty_discovery - range:" << (!range.empty() ? range : "none") << ", agent" << agent;

    Discovery server;

    // configure actor
    zactor_t* discovery_server = zactor_new(fty_discovery_server, NULL);
    if (agent) {
        zstr_sendx(discovery_server, REQ_BIND, FTY_DISCOVERY_ENDPOINT, FTY_DISCOVERY_ACTOR_NAME, NULL);
    } else {
        char* name = zsys_sprintf("%s.%i", FTY_DISCOVERY_ACTOR_NAME, getpid());
        zstr_sendx(discovery_server, REQ_BIND, FTY_DISCOVERY_ENDPOINT, name, NULL);
        zstr_free(&name);
    }
    zstr_sendx(discovery_server, REQ_CONFIG, config, NULL);
    zstr_sendx(discovery_server, REQ_CONSUMER, FTY_PROTO_STREAM_ASSETS, ".*", NULL);
    if (!range.empty()) {
        zstr_sendx(discovery_server, REQ_SCAN, range, NULL);
    } else if (!agent) {
        zstr_sendx(discovery_server, REQ_LOCALSCAN, NULL);
    }

    // main loop
    while (!zsys_interrupted) {
        zmsg_t* msg = zmsg_recv(discovery_server);
        if (msg) {
            char* cmd = zmsg_popstr(msg);
            log_debug("main: %s command received", cmd ? cmd : "(null)");
            if (cmd) {
                if (!agent && streq(cmd, REQ_DONE)) {
                    zstr_free(&cmd);
                    zmsg_destroy(&msg);
                    break;
                }
                zstr_free(&cmd);
            }
            zmsg_destroy(&msg);
        }
    }
    zactor_destroy(&discovery_server);
    return 0;
}
