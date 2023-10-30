# clap-remote

clap-remote is a plugin layer abstraction for the [**CL**ever **A**udio
**P**lugin](https://github.com/free-audio/clap) standard. It sits on-top of the
CLAP ABI and provides a high performance gRPC Server for clients to subscribe
to event streams ultimately allowing for remote development.

The latest public server API is available [here](api/v0/api.proto). To
create a client implementation visit the official [gRPC
Documentations](https://grpc.io/about/). Following lanmguages are supported:

|                           |   **Supported Languages** |                           |
|---------------------------|------------------------------------------------|--------------------------|
| 🛠️ [C++](https://grpc.io/docs/languages/cpp/)        | ☕ [Java](https://grpc.io/docs/languages/java/) | 🌐 [Node](https://grpc.io/docs/languages/node/)              |
| 🐍 [Python](https://grpc.io/docs/languages/python/)  | #  [C#](https://grpc.io/docs/languages/csharp/) | 🍏 [Objective-C](https://grpc.io/docs/languages/objective-c/) |
| 📦 [Go](https://grpc.io/docs/languages/go/)          | 🎯 [Dart](https://grpc.io/docs/languages/dart/) | 🐘 [PHP](https://grpc.io/docs/languages/php/)                |
| 🅺 [Kotlin](https://grpc.io/docs/languages/kotlin/)   | 💎 [Ruby](https://grpc.io/docs/languages/ruby/) |                                                              |


By utilizing [Protobuf](https://protobuf.dev/) for the communication protocol
and g**R**emote**P**rocedure**C**alls, we can achieve a fast communication
between the server and its clients. The server works by initializing
event-streams with the plugins to subscribe to, which will deliver all the
events. They are also used to communicate with the core-plugin instance
resulting in a server <-> clients architecture.


> ⚠️ **Development Notice** ⚠️: This project is actively evolving. The API may
> undergo changes without prior notice. Many aspects remain to be refined for
> optimal integration. We invite collaboration and feedback. If you're
> passionate about this topic, please reach out.

## Getting started

It is recommended to [install
gRPC](https://github.com/grpc/grpc/blob/master/BUILDING.md) system/user wide.
Please check you platform on how to install libraries and make them available.
This typically includes settings the `$PATH` variable and making sure the
library is found either by installing it to the default search path of the
system or setting the
[CMAKE_PREFIX_PATH](https://cmake.org/cmake/help/latest/variable/CMAKE_PREFIX_PATH.html)
variable.

```bash
https://github.com/deeedob/clap-remote.git --depth 1
git submodule update --init --recursive
cd clap-remote
mkdir build && cd build && cmake .. && ninja
```

The tests depend on [Catch2](https://github.com/catchorg/Catch2).

## How does it work?

We use one server that is local to the process, i.e. there will be only one
server instance which communicates with all the plugins.

