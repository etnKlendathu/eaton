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
        src/fty_discovery.cc
        src/fty_discovery_classes.h
        src/assets.h
        src/cidr.h
        src/server.cpp
        src/server.h
        src/commands.h

        src/scan/range.cpp
        src/scan/range.h
        src/scan/device.cpp
        src/scan/device.h
        src/scan/dns.cpp
        src/scan/dns.h
        src/scan/nut.cpp
        src/scan/nut.h

        src/wrappers/mlm.cpp
        src/wrappers/mlm.h
        src/wrappers/zmessage.h
        src/wrappers/actor.h
        src/wrappers/ftyproto.cpp
        src/wrappers/ftyproto.h
        src/wrappers/poller.cpp
        src/wrappers/poller.h

        src/discovery/discovery-impl.cpp
        src/discovery/discovery-impl.h
        src/discovery/serverconfig.cpp
        src/discovery/serverconfig.h
        src/discovery/discovered-devices.cpp
        src/discovery/discovered-devices.h
    INCLUDE_DIRS src
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

