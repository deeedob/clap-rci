find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC REQUIRED)

set(proto_out "${CMAKE_CURRENT_BINARY_DIR}")
add_library(proto-client OBJECT ${clap-remote_PROTO})
target_include_directories(proto-client PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(proto-client PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

# https://github.com/protocolbuffers/protobuf/blob/main/docs/cmake_protobuf_generate.md
protobuf_generate(
        TARGET proto-client
        IMPORT_DIRS "${clap-remote_PROTO_INCLUDE}"
        PROTOC_OUT_DIR "${proto_out}"
)

protobuf_generate(
        TARGET proto-client
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
        IMPORT_DIRS "${clap-remote_PROTO_INCLUDE}"
        PROTOC_OUT_DIR "${proto_out}"
)


add_executable(client-cpp client-cpp.cpp)
target_link_libraries(client-cpp PRIVATE proto-client core)
