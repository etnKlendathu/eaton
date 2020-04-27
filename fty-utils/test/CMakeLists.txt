find_package(Catch2 REQUIRED PATHS ${RUNTIME_PREFIX})
include(CTest)
include(Catch)

raven_target(exe ${PROJECT_NAME}-test
    SOURCES
        main.cpp
        split.cpp
        convert.cpp
        expected.cpp
        events.cpp
    USES
        ${PROJECT_NAME}
        Catch2::Catch2
        pthread
)

catch_discover_tests(${PROJECT_NAME}-test)
