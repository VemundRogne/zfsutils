cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/CPM.cmake)

enable_testing()

CPMAddPackage(
    NAME zfs
    GIT_REPOSITORY ssh://desktopserver/home/vemund/git/zfs.git
    GIT_TAG zfs-2.3.4-grpc-compatible
    DOWNLOAD_ONLY
)

execute_process(
    COMMAND "./autogen.sh"
    WORKING_DIRECTORY ${zfs_SOURCE_DIR}
)
execute_process(
    COMMAND "./configure"
    WORKING_DIRECTORY ${zfs_SOURCE_DIR}
)
execute_process(
    COMMAND "make"
    WORKING_DIRECTORY ${zfs_SOURCE_DIR}
)

project(zfsutils CXX)

add_library(zfsutils ./src/pool.cpp ./src/dataset.cpp ./src/snapshot.cpp)
target_include_directories(zfsutils PUBLIC ./inc)


# Link to ZFS
target_include_directories(zfsutils PUBLIC ${zfs_SOURCE_DIR}/include)
target_include_directories(zfsutils PUBLIC ${zfs_SOURCE_DIR}/lib/libspl/include)
target_include_directories(zfsutils PUBLIC ${zfs_SOURCE_DIR}/lib/libspl/include/os/linux)

target_link_directories(zfsutils PUBLIC ${zfs_SOURCE_DIR}/.libs)
target_link_libraries(zfsutils PUBLIC zfs)
target_link_libraries(zfsutils PUBLIC nvpair)

add_library(testutils ./test/testutils.cpp)
target_include_directories(testutils PUBLIC ${CMAKE_CURRENT_LIST_DIR}/test)

# We need some tests:
# for example memory-leak-checks: 'valgrind --leak-check=full ./build/binary.exe'

add_executable(test_core.exe ./test/test_core.cpp)
target_link_libraries(test_core.exe PUBLIC zfsutils)

add_test(NAME test_core COMMAND test_core.exe)

add_executable(test_piper.exe ./test/test_piper.cpp)
target_link_libraries(test_piper.exe PUBLIC zfsutils)
add_test(NAME test_piper COMMAND test_piper.exe)

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


add_executable(test_pool.exe ./test/test_pool.cpp)
target_link_libraries(test_pool.exe PUBLIC zfsutils)

add_test(NAME test_pool COMMAND test_pool.exe)
set_tests_properties(test_pool PROPERTIES FIXTURES_REQUIRED testpools)

# We add a test that should create a dataset in zfsutils_testpool_A:
add_executable(test_create_dataset.exe ./test/test_create_dataset.cpp)
target_link_libraries(test_create_dataset.exe PUBLIC zfsutils)

add_test(NAME test_create_dataset COMMAND test_create_dataset.exe)
set_tests_properties(test_create_dataset PROPERTIES FIXTURES_REQUIRED testpools)
set_tests_properties(test_create_dataset PROPERTIES FIXTURES_SETUP pool_A_testDataset)

# We add a test that should make snapshots on the pool_a_testDataset
add_executable(test_create_snapshots.exe ./test/test_create_snapshots.cpp)
target_link_libraries(test_create_snapshots.exe PUBLIC zfsutils testutils)

add_test(NAME test_create_snapshots COMMAND test_create_snapshots.exe)
set_tests_properties(test_create_snapshots PROPERTIES FIXTURES_REQUIRED "pool_A_testDataset;testpools") # Hmm... I do not love that I have to specify "testpools" as a required fixture here!
set_tests_properties(test_create_snapshots PROPERTIES FIXTURES_SETUP pool_A_testSnapshots)

# We add a test that should send snapshots from pool_a to pool_b
add_executable(test_send_snapshots.exe ./test/test_send_snapshots.cpp)
target_link_libraries(test_send_snapshots.exe PUBLIC zfsutils)

add_test(NAME test_send_snapshots COMMAND test_send_snapshots.exe)
set_tests_properties(test_send_snapshots PROPERTIES FIXTURES_REQUIRED "pool_A_testDataset;testpools;pool_A_testSnapshots") # Hmm... I do not love that I have to specify "testpools" as a required fixture here!

