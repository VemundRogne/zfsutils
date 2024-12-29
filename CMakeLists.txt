cmake_minimum_required(VERSION 3.25)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)

include(cmake/CPM.cmake)

project(zfs_test CXX)

add_subdirectory(zfsutils deps/zfsutils)

add_executable(test.exe ./test.cpp)
target_link_libraries(test.exe zfsutils)

