cmake_minimum_required(VERSION 3.10)
project(IfNetworkTools C)

set(CMAKE_C_STANDARD 99)
add_definitions(-D_POSIX_C_SOURCE=200809L)

add_library(ifproc_lib STATIC src/ifproc.c)

add_executable(ifshow src/ifshow.c)
target_link_libraries(ifshow ifproc_lib)

add_executable(agent src/agent.c)
target_link_libraries(agent ifproc_lib)

add_executable(ifnetshow src/ifnetshow.c common.h)

add_executable(neighborshow src/neighborshow.c common.h)

add_custom_target(projet_integrateur DEPENDS ifshow agent ifnetshow neighborshow)