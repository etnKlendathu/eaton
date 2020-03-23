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
    Continue,
    InfoReady
};

}

namespace fty {

template<typename T>
T convert(discovery::Command value)
{
    if constexpr (std::is_constructible_v<std::string, T>) {
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
    } else if constexpr (std::is_integral_v<T>) {
        return static_cast<T>(value);
    } else {
        static_assert(fty::always_false<T>, "Unsupported type");
    }
}

template<typename T>
discovery::Command convert(const T& value)
{
    if constexpr (std::is_constructible_v<T, std::string>) {
        std::string str = value;
        if (str == "BIND") {
            return discovery::Command::Bind;
        } else if (str == "DONE") {
            return discovery::Command::Done;
        } else if (str == "SCAN") {
            return discovery::Command::Scan;
        } else if (str == "$TERM") {
            return discovery::Command::Term;
        } else if (str == "FOUND") {
            return discovery::Command::Found;
        } else if (str == "CONFIG") {
            return discovery::Command::Config;
        } else if (str == "CONSUMER") {
            return discovery::Command::Consumer;
        } else if (str == "PROGRESS") {
            return discovery::Command::Progress;
        } else if (str == "STOPSCAN") {
            return discovery::Command::StopScan;
        } else if (str == "GETCONFIG") {
            return discovery::Command::GetConfig;
        } else if (str == "LOCALSCAN") {
            return discovery::Command::LocalScan;
        } else if (str == "REPUBLISH") {
            return discovery::Command::Republish;
        } else if (str == "SETCONFIG") {
            return discovery::Command::SetConfig;
        } else if (str == "LAUNCHSCAN") {
            return discovery::Command::LaunchScan;
        } else if (str == "CONTINUE") {
            return discovery::Command::Continue;
        } else if (str == "READY") {
            return discovery::Command::InfoReady;
        }
        throw std::runtime_error(str + " is wrong command");
    } else if constexpr (std::is_integral_v<T>) {
        return static_cast<discovery::Command>(value);
    } else {
        static_assert(fty::always_false<T>, "Unsupported type");
    }
}

}

