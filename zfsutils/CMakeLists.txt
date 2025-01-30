cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/CPM.cmake)

enable_testing()

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

# We add two fixtures:
#  - zpool_image_file (just a file that can host a testpool)
#  - testpool         (the testpool that we do test on)

# ctest must be run with sudo

add_test(NAME make_image_file  COMMAND "truncate" "-s" "2G" "${CMAKE_BINARY_DIR}/test_file.img")
set_tests_properties(make_image_file PROPERTIES FIXTURES_SETUP zpool_image_file)

add_test(NAME make_testpool COMMAND "zpool" "create" "zfsutils_testpool" "${CMAKE_BINARY_DIR}/test_file.img")
set_tests_properties(make_testpool PROPERTIES FIXTURES_REQUIRED zpool_image_file)
set_tests_properties(make_testpool PROPERTIES FIXTURES_SETUP testpool)

add_test(NAME cleanup_testpool COMMAND "zpool" "destroy" "zfsutils_testpool")
set_tests_properties(cleanup_testpool PROPERTIES FIXTURES_CLEANUP testpool)

