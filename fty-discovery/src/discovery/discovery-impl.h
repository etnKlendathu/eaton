#pragma once
#include "assets.h"
#include "discovery/discovered-devices.h"
#include "discovery/serverconfig.h"
#include "scan/range.h"
#include "server.h"
#include "wrappers/actor.h"
#include "wrappers/mlm.h"
#include <fty_common_db_defs.h>

// ===========================================================================================================

class Poller;

// ===========================================================================================================

class DiscoveryConfig : public Config
{
public:
    const std::string& fileName() const;
    void               load(const std::string& file);

private:
    std::string m_fileName;
};

// ===========================================================================================================

class Discovery::Impl : public Actor<Discovery::Impl>
{
public:
    Impl()            = default;
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    ~Impl() override;
    void runWorker();
    void bind(const std::string& name);

private:
    void reset();

    bool computeIpList(pack::StringList& listIp);

    void configureLocalScan();

    bool computeConfigurationFile();

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
    int64_t                               m_nbPercent        = 0;
    int64_t                               m_nbDiscovered     = 0;
    int64_t                               m_scanSize         = 0;
    int64_t                               m_nbUpsDiscovered  = 0;
    int64_t                               m_nbEpduDiscovered = 0;
    int64_t                               m_nbStsDiscovered  = 0;
    discovery::Status                     m_statusScan;
    bool                                  m_ongoingStop = false;
    std::vector<std::string>              m_localScanSubScan;
    fty::scan::RangeScan::Ranges          m_rangeScanConfig;
    ConfigurationScan                     m_configurationScan;
    std::unique_ptr<fty::scan::RangeScan> m_rangeScanner;
    std::string                           m_percent;
    fty::DiscoveredDevices                m_devicesDiscovered;
    fty::nut::KeyValues                   m_nutMappingInventory;
    std::vector<link_t>                   m_defaultValuesLinks;
    DiscoveryConfig                       m_config;
};

// ===========================================================================================================
