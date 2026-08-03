cmake_minimum_required(VERSION 3.20)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/CPM.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_COMPILER g++-14)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../core _deps/zfsutils_core)

project(snapper CXX)

CPMAddPackage("gh:gabime/spdlog@1.15.3")
CPMAddPackage(
    NAME CLI11
    GITHUB_REPOSITORY CLIUtils/CLI11
    VERSION 2.6.2
)

add_executable(snapper ./snapper.cpp)
target_link_libraries(snapper PRIVATE
    CLI11
    spdlog::spdlog
    zfsutils
)
