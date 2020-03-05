cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

ExternalProject_Add(catch2-build
    URL           https://github.com/catchorg/Catch2/archive/v2.11.1.tar.gz
    CMAKE_ARGS    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                  -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                  -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
                  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                  -DCMAKE_INSTALL_MESSAGE=LAZY
                  -DCATCH_BUILD_TESTING=OFF
                  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)
