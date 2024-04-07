# clap-rci

The **Remote Control Interface (RCI)** is a plugin layer abstraction for the
[*CLAP*](https://github.com/free-audio/clap) standard. It uses a
high-performance gRPC server to allow clients to connect and communicate
remotely with hosted plugins. This communication is facilitated through
protobuf and its language-agnostic
[IDL](https://en.wikipedia.org/wiki/Interface_description_language). As a
result, clients can be implemented in any of the supported programming
languages by utilizing the provided [api](api/v0/api.proto).

<div align="center" width="100%">
    <img width=77% src="https://github.com/deeedob/thesis/blob/main/images/clean/clap-rci_arch.png" />
</div>

## Prerequisite

It is recommended to [install
gRPC](https://github.com/grpc/grpc/blob/master/BUILDING.md) system/user wide.
Refer to your platform's documentation for install instructions.

The tests depend on [Catch2](https://github.com/catchorg/Catch2).

## Client Implementations

- [qtcleveraudioplugin](https://code.qt.io/cgit/playground/qtcleveraudioplugin.git/about/)
