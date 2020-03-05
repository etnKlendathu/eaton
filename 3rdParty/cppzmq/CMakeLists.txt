cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

ExternalProject_Add(cppzmq
    URL             https://github.com/zeromq/cppzmq/archive/v4.6.0.tar.gz
    CMAKE_ARGS     -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                   -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                   -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                   -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
                   -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                   -DCMAKE_INSTALL_MESSAGE=LAZY
                   -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)
