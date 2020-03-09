cmake_minimum_required(VERSION 3.10)
cmake_policy(VERSION 3.10)

##############################################################################################################

project(fty-discovery VERSION 1.0.0)

##############################################################################################################

find_package(raven-cmake PATHS ${CMAKE_BINARY_DIR})
find_package(fty-protoc PATHS ${CMAKE_BINARY_DIR})

##############################################################################################################

raven_target(exe ${PROJECT_NAME}
    PUBLIC
        include/fty_discovery.h
        include/fty_discovery_library.h
        include/fty_discovery_server.h

    SOURCES
        src/assets.cc
        src/cidr.cc
        src/device_scan.cc
        src/fty_discovery.cc
        src/fty_discovery_classes.h
        src/fty_discovery_selftest.cc
        src/range_scan.h  scan_dns.h
        src/scan_nut.h
        src/assets.h
        src/cidr.h
        src/device_scan.h
        src/fty_discovery_private_selftest.cc
        src/fty_discovery_server.cc
        src/range_scan.cc
        src/scan_dns.cc
        src/scan_nut.cc
        src/wrappers/mlm.cpp
        src/wrappers/mlm.h
    USES
        fty-utils
        pack
        libcidr
        libmlm
        fty-common-log
        fty-proto
)
