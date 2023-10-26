# Generate gRPC files
set(proto_out "${CMAKE_CURRENT_BINARY_DIR}/api")

add_library(proto-objects OBJECT ${proto_file})
target_include_directories(proto-objects PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(proto-objects PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

# https://github.com/protocolbuffers/protobuf/blob/main/docs/cmake_protobuf_generate.md
protobuf_generate(
    TARGET proto-objects
    IMPORT_DIRS ${proto_include}
    PROTOC_OUT_DIR "${proto_out}"
)

protobuf_generate(
    TARGET proto-objects
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
    IMPORT_DIRS "${proto_include}"
    PROTOC_OUT_DIR "${proto_out}"
)

set_property(SOURCE ${proto_out}/api.grpc.pb.cc PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE ${proto_out}/api.grpc.pb.h PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE ${proto_out}/api.pb.cc PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE ${proto_out}/api.pb.h PROPERTY SKIP_AUTOGEN ON)

