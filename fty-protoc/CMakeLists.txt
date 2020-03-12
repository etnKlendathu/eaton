cmake_minimum_required(VERSION 3.10)
cmake_policy(VERSION 3.10)

find_package(raven-cmake QUIET PATHS ${CMAKE_BINARY_DIR})

project(fty-protoc VERSION 1.0.0)

raven_target(exe ${PROJECT_NAME}
    SOURCES
        src/main.cpp
        src/generator.h
        src/generator.cpp
        src/filegenerator.h
        src/filegenerator.cpp
        src/classgenerator.h
        src/classgenerator.cpp
        src/formatter.h
        src/utils.h
    CMAKE
        cmake/protogen.cmake
    PREPROCESSOR
        -DPROTOBUF_USE_DLLS
    USES
        protobuf::libprotobuf
        protobuf::libprotoc
)

##############################################################################################################

# Proto requires strong naming for generator plugin
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME protoc-gen-fty)

##############################################################################################################

raven_define_component(${PROJECT_NAME}-dev
    TARGETS       ${PROJECT_NAME}
    BUILD_DEPENDS "protobuf"
    DEPENDS       "cmake (>= 3.10)"
)

raven_define_component(${PROJECT_NAME}-runtime
    SHLIBDEPS
    TARGETS       ${PROJECT_NAME}
)

raven_pack(
    PKG_NAME    fty-protoc
    CONTACT     "zjes <zjesclean@gmail.com>"
    COMPONENTS  ${PROJECT_NAME}-dev ${PROJECT_NAME}-runtime
    DESCRIPTION "Some useful utilites"
)

##############################################################################################################

