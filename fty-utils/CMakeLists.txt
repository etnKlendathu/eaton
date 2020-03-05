cmake_minimum_required(VERSION 3.10)
cmake_policy(VERSION 3.10)

project(fty-utils VERSION 1.0.0)

find_package(raven-cmake PATHS ${CMAKE_BINARY_DIR})

raven_target(interface ${PROJECT_NAME}
    PUBLIC
        utils/command-line.h
        utils/convert.h
        utils/split.h
        utils/traits.h
)

add_subdirectory(test)
