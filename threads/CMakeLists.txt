cmake_minimum_required(VERSION 3.10)
cmake_policy(VERSION 3.10)

##############################################################################################################

project(threads VERSION 1.0.0)

##############################################################################################################

find_package(raven-cmake PATHS ${CMAKE_BINARY_DIR})

##############################################################################################################

raven_target(exe ${PROJECT_NAME}
    SOURCES
        main.cpp
    USES
        fty-utils
        pthread
)
