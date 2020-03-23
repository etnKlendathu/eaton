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

    SOURCES
        src/assets.cc
        src/cidr.cc
        src/device_scan.cc
        src/fty_discovery.cc
        src/fty_discovery_classes.h
        src/range_scan.h  scan_dns.h
        src/scan_nut.h
        src/assets.h
        src/cidr.h
        src/device_scan.h
        src/fty_discovery_server.cc
        src/fty_discovery_server.h
        src/range_scan.cc
        src/scan_dns.cc
        src/scan_nut.cc
        src/commands.h

        src/wrappers/mlm.cpp
        src/wrappers/mlm.h
        src/wrappers/zmessage.h
        src/wrappers/actor.h
        src/wrappers/ftyproto.cpp
        src/wrappers/ftyproto.h
        src/wrappers/poller.cpp
        src/wrappers/poller.h
    USES
        pack
        libcidr
        libmlm
        libfty_proto
        libfty_common_db
        fty-common-logging
)


fty_protogen(
    TARGET ${PROJECT_NAME}
    WORKDIR proto
    PROTO
        proto/config.proto
        proto/default-values.proto
)

