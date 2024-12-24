cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CPM.cmake)

project(zfs_test CXX)

add_subdirectory(zfsutils deps/zfsutils)

add_executable(test.exe ./test.cpp)
target_link_libraries(test.exe zfsutils)

