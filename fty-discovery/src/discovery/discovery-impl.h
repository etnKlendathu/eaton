#pragma once
#include "assets.h"
#include "discovery/discovered-devices.h"
#include "discovery/serverconfig.h"
#include "server.h"
#include "scan/range.h"
#include "wrappers/actor.h"
#include "wrappers/mlm.h"
#include <fty_common_db_defs.h>

class Poller;

class Discovery::Impl : public Actor<Discovery::Impl>
{
public:
    ~Impl() override;
    void runWorker();

private:

    void reset();

    bool computeIpList(pack::StringList& listIp);

    void configureLocalScan();

    bool computeConfigurationFile(const std::string& config);

    void createAsset(ZMessage&& msg);

    void handlePipeBind(ZMessage&& message);

    void handlePipeConsumer(ZMessage&& message);

    void handlePipeConfig(ZMessage&& message);

    void handlePipeScan(ZMessage&& message, Poller& poll);

    void handlePipeLocalScan(Poller& poll);

    bool handlePipe(ZMessage&& message, Poller& poller);

    void launchScan(ZMessage&& msg, Poller& poller);

    void handleProgress(ZMessage&& msg);

    void handleStopScan(ZMessage&& msg);

    void handleMailbox(ZMessage&& msg, Poller& poller);

    void handleStream(ZMessage&& message);

    void handleRangeScanner(ZMessage&& msg, Poller& poll);

    void rangeScannerNew();

private:
    struct ConfigurationScan
    {
        std::vector<std::string> scan_list;
        int64_t                  scan_size;
        Config::Discovery::Type  type;
    };

private:
    Mlm                                   m_mlm;
    Mlm                                   m_mlmCreate;
    Assets                                m_assets;
    int64_t                               m_nbPercent;
    int64_t                               m_nbDiscovered;
    int64_t                               m_scanSize;
    int64_t                               m_nbUpsDiscovered;
    int64_t                               m_nbEpduDiscovered;
    int64_t                               m_nbStsDiscovered;
    discovery::Status                     m_statusScan;
    bool                                  m_ongoingStop;
    std::vector<std::string>              m_localScanSubScan;
    fty::scan::RangeScan::Ranges          m_rangeScanConfig;
    ConfigurationScan                     m_configurationScan;
    std::unique_ptr<fty::scan::RangeScan> m_rangeScanner;
    std::string                           m_percent;
    fty::DiscoveredDevices                m_devicesDiscovered;
    fty::nut::KeyValues                   m_nutMappingInventory;
    std::vector<link_t>                   m_defaultValuesLinks;
    Config                                m_config;
};
