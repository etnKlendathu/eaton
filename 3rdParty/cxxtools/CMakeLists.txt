cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

ExternalProject_Add(cxxtools
    GIT_REPOSITORY    https://github.com/42ity/cxxtools.git
    GIT_TAG           "2.2-FTY-master"
    SOURCE_DIR        ${CMAKE_CURRENT_BINARY_DIR}/src
    CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/src/autogen.sh 
                      &&
                      ${CMAKE_CURRENT_BINARY_DIR}/src/configure 
                        CXX=${CMAKE_CXX_COMPILER}
                        CC=${CMAKE_C_COMPILER}
                        PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}
                        --prefix=${CMAKE_INSTALL_PREFIX}
    BUILD_COMMAND     make -j 5
    BUILD_IN_SOURCE   1
)
