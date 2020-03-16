cmake_minimum_required(VERSION 3.10)
cmake_policy(VERSION 3.10)

##############################################################################################################

project(pack VERSION 1.0.0)

##############################################################################################################

find_package(raven-cmake PATHS ${CMAKE_BINARY_DIR})
find_package(fty-protoc PATHS ${CMAKE_BINARY_DIR})

##############################################################################################################

raven_target(shared ${PROJECT_NAME}
    PUBLIC
        pack/pack.h
        pack/attribute.h
        pack/list.h
        pack/map.h
        pack/value.h
        pack/enum.h
        pack/node.h
        pack/magic_enum.h
        pack/types.h
        pack/serialization.h
        pack/proto-map.h

    SOURCES
        src/node.cpp
        src/attribute.cpp
        src/providers/visitor.h
        src/providers/yaml.cpp
        src/providers/zconfig.cpp
        src/providers/protobuf.cpp
        src/providers/serialization.cpp
    USES_PUBLIC
        fty-utils
    USES
        yaml-cpp
        libczmq
        protobuf::libprotobuf
)

##############################################################################################################

raven_target(exe ${PROJECT_NAME}-test
    SOURCES
        tests/main.cpp
        tests/simple.cpp
        tests/list.cpp
        tests/nested.cpp
        tests/enums.cpp
        tests/map.cpp
        tests/include.cpp
        tests/oneof.cpp
    PREPROCESSOR -DCATCH_CONFIG_FAST_COMPILE
    USES
        pack
        Catch2::Catch2
)

fty_protogen(
    TARGET ${PROJECT_NAME}-test
    WORKDIR tests
    PROTO
        tests/examples/example1.proto
        tests/examples/example2.proto
        tests/examples/example3.proto
        tests/examples/example4.proto
        tests/examples/example5.proto
        tests/examples/example6.proto
        tests/examples/example7.proto
        tests/examples/example8.proto
)

##############################################################################################################

raven_define_component(${PROJECT_NAME}-dev
    TARGETS ${PROJECT_NAME}
)

raven_define_component(${PROJECT_NAME}-runtime
    SHLIBDEPS
    TARGETS ${PROJECT_NAME}
)

raven_pack(
    PKG_NAME        "fty-pack"
    CONTACT         "zjesclean@gmail.com"
    COMPONENTS      ${PROJECT_NAME}-dev ${PROJECT_NAME}-runtime
    DESCRIPTION     "Simple cmake wrapper to easy target creation, packing"
    LD_LIBRARY_PATH "${RUNTIME_PREFIX}/lib"
)

##############################################################################################################
