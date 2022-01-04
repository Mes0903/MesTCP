SET(CMAKE_TOOLCHAIN_FILE "C:/Users/Mes/vcpkg/scripts/buildsystems/vcpkg.cmake")
cmake_minimum_required(VERSION 3.21)
project(MesServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(MesServer ${SOURCE_FILES})
target_include_directories(MesServer PRIVATE include)
target_link_libraries(MesServer
                ${Boost_LIBRARIES}
                Threads::Threads)