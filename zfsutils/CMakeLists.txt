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
#  - zpool_image_files (just two that can host a testpool)
#  - testpools         (the testpools that we do test on)   depends on zpool_image_files
# testpools fixture creates two zpools: "zfsutils_testpool_A" "zfsutils_testpool_B"

# ctest must be run with sudo

add_test(NAME make_image_file_A COMMAND "truncate" "-s" "2G" "${CMAKE_BINARY_DIR}/test_file_A.img")
add_test(NAME make_image_file_B COMMAND "truncate" "-s" "2G" "${CMAKE_BINARY_DIR}/test_file_B.img")
set_tests_properties(make_image_file_A PROPERTIES FIXTURES_SETUP zpool_image_files)
set_tests_properties(make_image_file_B PROPERTIES FIXTURES_SETUP zpool_image_files)

add_test(NAME make_testpool_A COMMAND "zpool" "create" "zfsutils_testpool_A" "${CMAKE_BINARY_DIR}/test_file_A.img")
set_tests_properties(make_testpool_A PROPERTIES FIXTURES_REQUIRED zpool_image_files)
set_tests_properties(make_testpool_A PROPERTIES FIXTURES_SETUP testpools)

add_test(NAME make_testpool_B COMMAND "zpool" "create" "zfsutils_testpool_B" "${CMAKE_BINARY_DIR}/test_file_B.img")
set_tests_properties(make_testpool_B PROPERTIES FIXTURES_REQUIRED zpool_image_files)
set_tests_properties(make_testpool_B PROPERTIES FIXTURES_SETUP testpools)

add_test(NAME cleanup_testpool_A COMMAND "zpool" "destroy" "zfsutils_testpool_A")
add_test(NAME cleanup_testpool_B COMMAND "zpool" "destroy" "zfsutils_testpool_B")
set_tests_properties(cleanup_testpool_A PROPERTIES FIXTURES_CLEANUP testpools)
set_tests_properties(cleanup_testpool_B PROPERTIES FIXTURES_CLEANUP testpools)

