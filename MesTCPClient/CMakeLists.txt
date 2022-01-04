SET(CMAKE_TOOLCHAIN_FILE "C:/Users/Mes/vcpkg/scripts/buildsystems/vcpkg.cmake")
cmake_minimum_required(VERSION 3.21)
project(MesClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(MesClient ${SOURCE_FILES})
target_include_directories(MesClient PRIVATE include)
target_link_libraries(MesClient
                ${Boost_LIBRARIES}
                Threads::Threads)