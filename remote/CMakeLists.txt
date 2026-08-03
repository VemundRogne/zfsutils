cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#set(CMAKE_CXX_COMPILER g++-14)
#set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../core _deps/zfsutils_core)

project(zfsutils_remote CXX)

list(PREPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local")
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

add_custom_command(
    COMMENT "Building protocols"
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/remotezfs.pb.cc ${CMAKE_CURRENT_BINARY_DIR}/remotezfs.grpc.pb.cc
    COMMAND $<TARGET_FILE:protobuf::protoc> 
        "${CMAKE_CURRENT_LIST_DIR}/protos/remotezfs.proto"
        "--grpc_out=${CMAKE_CURRENT_BINARY_DIR}"
        "--plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
        "--proto_path=${CMAKE_CURRENT_LIST_DIR}/protos"
    COMMAND $<TARGET_FILE:protobuf::protoc>
        "${CMAKE_CURRENT_LIST_DIR}/protos/remotezfs.proto"
        "--cpp_out=${CMAKE_CURRENT_BINARY_DIR}"
        "--proto_path=${CMAKE_CURRENT_LIST_DIR}/protos"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/protos/remotezfs.proto
)

add_library(remotezfs_proto
    ./remotezfs.pb.cc
    ./remotezfs.grpc.pb.cc
)

target_include_directories(remotezfs_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(remotezfs_proto PUBLIC gRPC::grpc++)

add_library(zfsutils_remote ./src/pool.cpp)
target_include_directories(zfsutils_remote PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
target_link_libraries(zfsutils_remote PUBLIC zfsutils_interface)
target_link_libraries(zfsutils_remote PUBLIC remotezfs_proto)

add_executable(test_pool_info.exe ./test/test_pool_info.cpp)
target_link_libraries(test_pool_info.exe PUBLIC zfsutils)
target_link_libraries(test_pool_info.exe PUBLIC remotezfs_proto)
target_link_libraries(test_pool_info.exe PUBLIC zfsutils_remote)

add_executable(test_pool_info_server.exe ./test/test_pool_info_server.cpp)
target_link_libraries(test_pool_info_server.exe PUBLIC zfsutils)
target_link_libraries(test_pool_info_server.exe PUBLIC remotezfs_proto)
