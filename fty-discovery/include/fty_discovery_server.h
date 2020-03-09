/*  =========================================================================
    ftydiscovery - Manages discovery requests, provides feedback

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

#pragma once
#include <mutex>
#include <pack/pack.h>

struct Config : public pack::Node
{
    using pack::Node::Node;
    struct Server : public pack::Node
    {
        using pack::Node::Node;
        pack::Int32  timeout    = FIELD("timeout");
        pack::Bool   background = FIELD("background");
        pack::String workdir    = FIELD("workdir");
        pack::Bool   verbose    = FIELD("verbose");
        META(Server, timeout, background, workdir, verbose)
    };

    struct Discovery : public pack::Node
    {
        using pack::Node::Node;

        struct Link : public pack::Node
        {
            using pack::Node::Node;

            pack::UInt32 src  = FIELD("src");
            pack::UInt32 type = FIELD("dest");

            META(Link, src, type)
        };

        enum class Type
        {
            localscan = 1,
            multiscan = 2,
            ipscan    = 3
        };

        pack::Enum<Type>       type               = FIELD("type");
        pack::StringList       scans              = FIELD("scans");
        pack::StringList       ips                = FIELD("ips");
        pack::String           scanNumber         = FIELD("scanNumber");
        pack::String           ipNumber           = FIELD("ipNumber");
        pack::StringList       documents          = FIELD("documents");
        pack::StringMap        defaultValuesAux   = FIELD("defaultValuesAux");
        pack::StringMap        defaultValuesExt   = FIELD("defaultValuesExt");
        pack::ObjectList<Link> defaultValuesLinks = FIELD("defaultValuesLinks");

        META(Discovery, type, scans, ips, scanNumber, ipNumber, documents, defaultValuesAux, defaultValuesExt,
            defaultValuesLinks)
    };

    struct Parameters : public pack::Node
    {
        using pack::Node::Node;

        pack::String mappingFile       = FIELD("mappingFile");
        pack::Int32  maxDumpPoolNumber = FIELD("maxDumpPoolNumber", 15);
        pack::Int32  maxScanPoolNumber = FIELD("maxScanPoolNumber", 4);
        pack::Int32  nutScannerTimeOut = FIELD("nutScannerTimeOut", 20);
        pack::Int32  dumpDataLoopTime  = FIELD("dumpDataLoopTime", 30);

        META(Parameters, mappingFile, maxDumpPoolNumber, maxScanPoolNumber, nutScannerTimeOut,
            dumpDataLoopTime)
    };

    struct Log : public pack::Node
    {
        using pack::Node::Node;

        pack::String config = FIELD("config", "/etc/fty/ftylog.cfg");

        META(Log, config)
    };

    Server     server     = FIELD("server");
    Discovery  discovery  = FIELD("discovery");
    Parameters parameters = FIELD("parameters");
    Log        log        = FIELD("log");

    META(Config, server, discovery, parameters, log)
};


#define DEFAULT_DUMPDATA_LOOP "2"

#define STATUS_STOPPED  1
#define STATUS_FINISHED 2
#define STATUS_PROGESS  3
#define STATUS_STOPPING "STOPPING"
#define STATUS_RUNNING  "RUNNING"

#define REQ_TERM       "$TERM"
#define REQ_BIND       "BIND"
#define REQ_CONSUMER   "CONSUMER"
#define REQ_REPUBLISH  "REPUBLISH"
#define REQ_SCAN       "SCAN"
#define REQ_LOCALSCAN  "LOCALSCAN"
#define REQ_SETCONFIG  "SETCONFIG"
#define REQ_GETCONFIG  "GETCONFIG"
#define REQ_CONFIG     "CONFIG"
#define REQ_LAUNCHSCAN "LAUNCHSCAN"
#define REQ_PROGRESS   "PROGRESS"
#define REQ_STOPSCAN   "STOPSCAN"
#define REQ_DONE       "DONE"
#define REQ_FOUND      "FOUND"
#define REQ_PROGRESS   "PROGRESS"

#define STREAM_CMD  "STREAM DELIVER"
#define MAILBOX_CMD "MAILBOX DELIVER"

#define RESP_OK  "OK"
#define RESP_ERR "ERROR"

#define INFO_READY   "READY"
#define CMD_CONTINUE "CONTINUE"

#define FTY_ASSET "asset-agent"

#define CREATE_USER "system"
#define CREATE_MODE "3"

typedef struct _discovered_devices_t
{
    std::mutex mtx_list;
    zhash_t*   device_list;
} discovered_devices_t;

Config& discoveryConfig();
