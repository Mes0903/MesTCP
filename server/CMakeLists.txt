SET(CMAKE_TOOLCHAIN_FILE "C:/Users/Mes/vcpkg/scripts/buildsystems/vcpkg.cmake")
cmake_minimum_required(VERSION 3.12)
project(MesTCPChatServer)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(MesTCPChatServer ${SOURCE_FILES})

target_include_directories(MesTCPChatServer PRIVATE include)
target_link_libraries(MesTCPChatServer
                ${Boost_LIBRARIES})