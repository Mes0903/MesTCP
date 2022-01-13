SET(CMAKE_TOOLCHAIN_FILE "C:/Users/Mes/vcpkg/scripts/buildsystems/vcpkg.cmake")
cmake_minimum_required(VERSION 3.12)
project(MesTCPChatClient)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(MesTCPChatClient ${SOURCE_FILES})

target_include_directories(MesTCPChatClient PRIVATE include)
target_link_libraries(MesTCPChatClient
                ${Boost_LIBRARIES})
