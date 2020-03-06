cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

ExternalProject_Add(tntdb
    GIT_REPOSITORY    https://github.com/42ity/tntdb.git
    GIT_TAG           "1.3-FTY-master"
    SOURCE_DIR        ${CMAKE_CURRENT_BINARY_DIR}/src
    SOURCE_SUBDIR     tntdb
    CONFIGURE_COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/src/tntdb/ && ./autogen.sh &&
                      ./configure
                        CXX=${CMAKE_CXX_COMPILER}
                        CC=${CMAKE_C_COMPILER}
                        PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}
                        --prefix=${CMAKE_INSTALL_PREFIX}
    BUILD_COMMAND     cd ${CMAKE_CURRENT_BINARY_DIR}/src/tntdb/ && make -j 5
    INSTALL_COMMAND   cd ${CMAKE_CURRENT_BINARY_DIR}/src/tntdb/ && make install
    BUILD_IN_SOURCE   1
)
