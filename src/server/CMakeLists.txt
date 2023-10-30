# Generate gRPC files
set(proto_out "${CMAKE_CURRENT_BINARY_DIR}/api/v0")

add_library(proto-objects OBJECT "${clap-remote_PROTO}")
target_include_directories(proto-objects PUBLIC "$<BUILD_INTERFACE:${proto_out}>")
target_link_libraries(proto-objects PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

# https://github.com/protocolbuffers/protobuf/blob/main/docs/cmake_protobuf_generate.md
protobuf_generate(
    TARGET proto-objects
    IMPORT_DIRS "${clap-remote_PROTO_INCLUDE}"
    PROTOC_OUT_DIR "${proto_out}"
)

protobuf_generate(
    TARGET proto-objects
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
    IMPORT_DIRS ${clap-remote_PROTO_INCLUDE}
    PROTOC_OUT_DIR ${proto_out}
)

set(generated_files
    ${proto_out}/api.grpc.pb.cc
    ${proto_out}/api.grpc.pb.h
    ${proto_out}/api.pb.cc
    ${proto_out}/api.pb.h
)
set_source_files_properties(${generated_files} PROPERTIES GENERATED TRUE)

set_property(SOURCE ${generated_files} PROPERTY SKIP_AUTOGEN ON)
