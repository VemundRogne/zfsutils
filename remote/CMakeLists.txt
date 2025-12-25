cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/CPM.cmake)

project(zfsutils_remote CXX)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../core _deps/zfsutils_core)

list(PREPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local")
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

add_custom_command(
    COMMENT "Building protocols"
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pool.pb.cc ${CMAKE_CURRENT_BINARY_DIR}/pool.grpc.pb.cc
    COMMAND $<TARGET_FILE:protobuf::protoc> 
        "${CMAKE_CURRENT_LIST_DIR}/protos/pool.proto"
        "--grpc_out=${CMAKE_CURRENT_BINARY_DIR}"
        "--plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
        "--proto_path=${CMAKE_CURRENT_LIST_DIR}/protos"
    COMMAND $<TARGET_FILE:protobuf::protoc>
        "${CMAKE_CURRENT_LIST_DIR}/protos/pool.proto"
        "--cpp_out=${CMAKE_CURRENT_BINARY_DIR}"
        "--proto_path=${CMAKE_CURRENT_LIST_DIR}/protos"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/protos/pool.proto
)

add_library(pool_proto
    ./pool.pb.cc
    ./pool.grpc.pb.cc
)

target_include_directories(pool_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(pool_proto PUBLIC gRPC::grpc++)

add_executable(test_pool_info.exe ./test/test_pool_info.cpp)
target_link_libraries(test_pool_info.exe PUBLIC zfsutils)
target_link_libraries(test_pool_info.exe PUBLIC pool_proto)

add_executable(test_pool_info_server.exe ./test/test_pool_info_server.cpp)
target_link_libraries(test_pool_info_server.exe PUBLIC zfsutils)
target_link_libraries(test_pool_info_server.exe PUBLIC pool_proto)
