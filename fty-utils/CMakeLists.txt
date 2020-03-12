cmake_minimum_required(VERSION 3.10)
cmake_policy(VERSION 3.10)

find_package(raven-cmake PATHS ${CMAKE_BINARY_DIR})

##############################################################################################################

project(fty-utils VERSION 1.0.0)

##############################################################################################################

raven_target(interface ${PROJECT_NAME}
    PUBLIC
        fty/command-line.h
        fty/convert.h
        fty/split.h
        fty/traits.h
        fty/expected.h
)

##############################################################################################################

raven_define_component(${PROJECT_NAME}-dev
    TARGETS       ${PROJECT_NAME}
    BUILD_DEPENDS "cmake (>= 3.10)"
    DEPENDS       "cmake (>= 3.10)"
)

raven_pack(
    PKG_NAME    fty-utils
    CONTACT     "zjes <zjesclean@gmail.com>"
    COMPONENTS  ${PROJECT_NAME}-dev
    DESCRIPTION "Some useful utilites"
)

##############################################################################################################

add_subdirectory(test)
