cmake_minimum_required(VERSION 3.10)
cmake_policy(VERSION 3.10)

project(FTY)

add_subdirectory(raven-cmake)
add_subdirectory(fty-utils)
add_subdirectory(fty-protoc)
add_subdirectory(fty-pack)

#add_subdirectory(fty-common-logging)
#add_subdirectory(fty-common-messagebus)
#add_subdirectory(fty-discovery)
