cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

ExternalProject_Add(yaml-cpp-build
    URL                 https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.3.tar.gz
    CMAKE_ARGS          -DYAML_CPP_BUILD_TESTS=OFF
                        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                        -DYAML_BUILD_SHARED_LIBS=OFF
                        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
                        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                        -DCMAKE_INSTALL_MESSAGE=LAZY
                        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

