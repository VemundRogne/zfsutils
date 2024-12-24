cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/CPM.cmake)

# This package now must be built by the user... Not very cool
CPMAddPackage(
    NAME zfs
    SOURCE_DIR /home/vemund/git/zfs
    DOWNLOAD_ONLY
)

project(zfsutils CXX)

add_library(zfsutils INTERFACE)
target_include_directories(zfsutils INTERFACE ./inc)


# Link to ZFS
target_include_directories(zfsutils INTERFACE ${zfs_SOURCE_DIR}/include)
target_include_directories(zfsutils INTERFACE ${zfs_SOURCE_DIR}/lib/libspl/include)
target_include_directories(zfsutils INTERFACE ${zfs_SOURCE_DIR}/lib/libspl/include/os/linux)

target_link_directories(zfsutils INTERFACE ${zfs_SOURCE_DIR}/.libs)
target_link_libraries(zfsutils INTERFACE zfs)
target_link_libraries(zfsutils INTERFACE nvpair)


# We need some tests:
# for example memory-leak-checks: 'valgrind --leak-check=full ./build/binary.exe'

