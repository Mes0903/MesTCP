cmake_minimum_required(VERSION 3.12)
project(MesTCPChat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_subdirectory(client)
add_subdirectory(server)
