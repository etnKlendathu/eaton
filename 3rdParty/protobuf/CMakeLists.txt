cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

set(args
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DBUILD_SHARED_LIBS=OFF
    -DCMAKE_INSTALL_MESSAGE=LAZY
    -Dprotobuf_BUILD_TESTS=OFF
    -Dprotobuf_BUILD_EXAMPLES=OFF
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

ExternalProject_Add(protobuf-build
    URL                 https://github.com/protocolbuffers/protobuf/archive/v3.11.4.tar.gz
    SOURCE_DIR          ${CMAKE_CURRENT_BINARY_DIR}/source
    CONFIGURE_COMMAND   ${CMAKE_COMMAND} ${args} ${CMAKE_CURRENT_BINARY_DIR}/source/cmake
)

