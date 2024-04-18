<div align="center">

# clap-rci

Remote Control Interface for the [*CLAP*](https://github.com/free-audio/clap)
audio-plugin standard.

</div>

Build audio-plugins with the provided [*C++ API*](include/clap-rci). It's a
thin layer around CLAP's C-API, providing additional convenience to build your
audio-plugin which directly interacts with the host's audio processing.

Start a single asynchronous gRPC server which manages all the plugins contained
within the CLAP. Clients can implement the [*protobuf API*](api/rci.proto).
Many different programming languages are supported.

Out-of-process, hell, even out-of-your-computer clients can then connect to the
plugins and enjoy a lively (perhaps friendly) communication with them. Send
events from the processing unit or receive them automagically.

## Prerequisite

It is recommended to [install
gRPC](https://github.com/grpc/grpc/blob/master/BUILDING.md) system/user wide.

The tests depend on [Catch2](https://github.com/catchorg/Catch2).

## Client Implementations

- [qtcleveraudioplugin](https://code.qt.io/cgit/playground/qtcleveraudioplugin.git/about/)
