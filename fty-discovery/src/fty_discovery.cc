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
#include "wrappers/zmessage.h"


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
    if (!server.init()) {
        return 1;
    }

    std::string name = Discovery::ActorName;
    if (!agent) {
        name += "."+ std::to_string(getpid());
    }
    server.runCommand(Discovery::Command::Bind, Discovery::Endpoint, name);
    server.runCommand(Discovery::Command::Config, config);
    server.runCommand(Discovery::Command::Consumer, FTY_PROTO_STREAM_ASSETS, ".*");

    if (!range.empty()) {
        server.runCommand(Discovery::Command::Scan, range);
    } else if (!agent) {
        server.runCommand(Discovery::Command::LocalScan);
    }

    // main loop
    while (!zsys_interrupted) {
        ZMessage msg = server.read();
        if (msg) {
            auto cmd = msg.popStr();
            logDbg() << "main:" << (cmd ? *cmd : "(null)") << "command received";
            if (cmd) {
                if (!agent && *cmd == REQ_DONE) {
                    break;
                }
            }
        }
    }
    return 0;
}
