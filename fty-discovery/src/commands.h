#pragma once
#include <fty/convert.h>

namespace discovery
{

enum class Command
{
    Term,
    Bind,
    Consumer,
    Republish,
    Scan,
    LocalScan,
    SetConfig,
    GetConfig,
    Config,
    LaunchScan,
    Progress,
    StopScan,
    Done,
    Found,
    FoundDc,
    NotFound,
    Continue,
    InfoReady
};

enum class Status
{
    Stopping,
    Running,
    Stopped,
    Finished,
    Progress
};

enum class Result
{
    Failed,
    Ok,
    Error
};

enum class Deliver
{
    Stream,
    MailBox
};

}

namespace fty {

template<>
inline std::string convert(const discovery::Command& value)
{
    switch (value) {
        case discovery::Command::Bind:
            return "BIND";
        case discovery::Command::Done:
            return "DONE";
        case discovery::Command::Scan:
            return "SCAN";
        case discovery::Command::Term:
            return "$TERM";
        case discovery::Command::Found:
            return "FOUND";
        case discovery::Command::FoundDc:
            return "FOUND_DC";
        case discovery::Command::NotFound:
            return "NOTFOUND";
        case discovery::Command::Config:
            return "CONFIG";
        case discovery::Command::Consumer:
            return "CONSUMER";
        case discovery::Command::Progress:
            return "PROGRESS";
        case discovery::Command::StopScan:
            return "STOPSCAN";
        case discovery::Command::GetConfig:
            return "GETCONFIG";
        case discovery::Command::LocalScan:
            return "LOCALSCAN";
        case discovery::Command::Republish:
            return "REPUBLISH";
        case discovery::Command::SetConfig:
            return "SETCONFIG";
        case discovery::Command::LaunchScan:
            return "LAUNCHSCAN";
        case discovery::Command::Continue:
            return "CONTINUE";
        case discovery::Command::InfoReady:
            return "READY";
    }
}

template<>
inline discovery::Command convert(const std::string& value)
{
    if (value == "BIND") {
        return discovery::Command::Bind;
    } else if (value == "DONE") {
        return discovery::Command::Done;
    } else if (value == "SCAN") {
        return discovery::Command::Scan;
    } else if (value == "$TERM") {
        return discovery::Command::Term;
    } else if (value == "FOUND") {
        return discovery::Command::Found;
    } else if (value == "FOUND_DC") {
        return discovery::Command::FoundDc;
    } else if (value == "NOTFOUND") {
        return discovery::Command::NotFound;
    } else if (value == "CONFIG") {
        return discovery::Command::Config;
    } else if (value == "CONSUMER") {
        return discovery::Command::Consumer;
    } else if (value == "PROGRESS") {
        return discovery::Command::Progress;
    } else if (value == "STOPSCAN") {
        return discovery::Command::StopScan;
    } else if (value == "GETCONFIG") {
        return discovery::Command::GetConfig;
    } else if (value == "LOCALSCAN") {
        return discovery::Command::LocalScan;
    } else if (value == "REPUBLISH") {
        return discovery::Command::Republish;
    } else if (value == "SETCONFIG") {
        return discovery::Command::SetConfig;
    } else if (value == "LAUNCHSCAN") {
        return discovery::Command::LaunchScan;
    } else if (value == "CONTINUE") {
        return discovery::Command::Continue;
    } else if (value == "READY") {
        return discovery::Command::InfoReady;
    }
    throw std::runtime_error(value + " is wrong command");
}

template<>
inline std::string convert(const discovery::Status& value)
{
    switch (value) {
        case discovery::Status::Running:
            return "RUNNING";
        case discovery::Status::Stopping:
            return "STOPPING";
        case discovery::Status::Stopped:
            return "STOPPED";
        case discovery::Status::Finished:
            return "FINISHED";
        case discovery::Status::Progress:
            return "PROGRESS";
    }
}

template<>
inline discovery::Status convert(const std::string& value)
{
    if (value == "RUNNING") {
        return discovery::Status::Running;
    } else if (value == "STOPPING") {
        return discovery::Status::Stopping;
    } else if (value == "STOPPED") {
        return discovery::Status::Stopped;
    } else if (value == "FINISHED") {
        return discovery::Status::Finished;
    } else if (value == "PROGRESS") {
        return discovery::Status::Progress;
    }
    throw std::runtime_error(value + " is wrong command");
}

template<>
inline std::string convert(const discovery::Result& value)
{
    switch (value) {
        case discovery::Result::Ok:
            return "OK";
        case discovery::Result::Error:
            return "ERROR";
        case discovery::Result::Failed:
            return "FAILED";
    }
}

template<>
inline discovery::Result convert(const std::string& value)
{
    if (value == "OK") {
        return discovery::Result::Ok;
    } else if (value == "ERROR") {
        return discovery::Result::Error;
    } else if (value == "FAILED") {
        return discovery::Result::Failed;
    }
    throw std::runtime_error(value + " is wrong command");
}

template<>
inline std::string convert(const discovery::Deliver& value)
{
    switch (value) {
        case discovery::Deliver::Stream:
            return "STREAM DELIVER";
        case discovery::Deliver::MailBox:
            return "MAILBOX DELIVER";
    }
}

template<>
inline discovery::Deliver convert(const std::string& value)
{
    if (value == "STREAM DELIVER") {
        return discovery::Deliver::Stream;
    } else if (value == "MAILBOX DELIVER") {
        return discovery::Deliver::MailBox;
    }
    throw std::runtime_error(value + " is wrong command");
}

}

