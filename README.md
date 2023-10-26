# clap-remote

clap-remote is a plugin layer abstraction for the [**CL**ever **A**udio
**P**lugin](https://github.com/free-audio/clap) standard. It sits on-top of the
CLAP ABI and provides a high performance gRPC Server for clients to subscribe
to event streams ultimately allowing for remote development.

The latest public server API is available [here](api/v0/api.proto). To
create a client implementation visit the official [gRPC
Documentations](https://grpc.io/about/). Following lanmguages are supported:

[C++](https://grpc.io/docs/languages/cpp/),
[Python](https://grpc.io/docs/languages/python/),
[Go](https://grpc.io/docs/languages/go/),
[Java](https://grpc.io/docs/languages/java/),
[C#](https://grpc.io/docs/languages/csharp/),
[Dart](https://grpc.io/docs/languages/dart/),
[Kotlin](https://grpc.io/docs/languages/kotlin/),
[Node](https://grpc.io/docs/languages/node/),
[Objective-C](https://grpc.io/docs/languages/objective-c/),
[PHP](https://grpc.io/docs/languages/php/),
[Ruby](https://grpc.io/docs/languages/ruby/)

By utilizing [Protobuf]() for the communication protocol and g**R**emote**P**rocedure**C**alls,
we can achieve a fast communication between the server and its clients. The server works by
initializing event-streams with the plugins to subscribe to, which will deliver all the events.
They are also used to communicate with the core-plugin instance resulting in a server <-> clients
architecture.

## How does it work?

We use one server that is local to the process, i.e. there will be only one
server instance which communicates with all the plugins.

