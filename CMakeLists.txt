cmake_minimum_required(VERSION 3.16)
project(hermes LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(./cmake/GrpcSetup.cmake)

# proto file
get_filename_component(page_proto "./protos/page.proto" ABSOLUTE)
get_filename_component(page_proto_path "./protos" PATH)

# generated sources
set(page_proto_srcs ${CMAKE_CURRENT_SOURCE_DIR}/genproto/protos/page.pb.cc)
set(page_proto_hdrs ${CMAKE_CURRENT_SOURCE_DIR}/genproto/protos/page.pb.h)
set(page_grpc_srcs ${CMAKE_CURRENT_SOURCE_DIR}/genproto/protos/page.grpc.pb.cc)
set(page_grpc_hdrs ${CMAKE_CURRENT_SOURCE_DIR}/genproto/protos/page.grpc.pb.h)
add_custom_command(
    OUTPUT "${page_proto_srcs}" "${page_proto_hdrs}" "${page_grpc_srcs}" "${page_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}/genproto"
    --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}/genproto"
    -I "${page_proto_path}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${page_proto}"
    DEPENDS "${page_proto}"
)

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/genproto")

# page_grpc_proto
add_library(page_grpc_proto
    ${page_grpc_srcs}
    ${page_grpc_hdrs}
    ${page_proto_srcs}
    ${page_proto_hdrs}
)
target_link_libraries(page_grpc_proto
        absl::check
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
)

target_include_directories(page_grpc_proto PUBLIC "${GEN_DIR}")
include_directories("./include")

add_executable(${PROJECT_NAME}
    src/runtime.cc
    src/server.cc
    src/allocator.cc
    main.cc
)

find_package(spdlog REQUIRED)

target_link_libraries(${PROJECT_NAME}
    page_grpc_proto
    spdlog::spdlog
)