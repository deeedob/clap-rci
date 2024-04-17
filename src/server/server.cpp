#include "pluginservice.hpp"

#include <clap-rci/server.h>

#include <grpc/event_engine/event_engine.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <absl/log/log.h>

#include <mutex>

CLAP_RCI_BEGIN_NAMESPACE

struct ServerPrivate {
    explicit ServerPrivate() = default;

    std::string address;
    int selectedPort = -1;
    std::unique_ptr<grpc::Server> server;
    std::mutex serverMtx;

    ClapPluginService pluginService;

    Server::State state = Server::State::Init;
};

Server::Server()
    : dPtr(std::make_unique<ServerPrivate>())
{
}

Server::~Server() = default;

bool Server::start(std::string_view addressUri)
{
    std::unique_lock lock(dPtr->serverMtx);

    // Server can be started only once
    if (dPtr->state != Server::State::Init)
        return false;

    {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(
            addressUri.data(), grpc::InsecureServerCredentials(),
            &dPtr->selectedPort
        );
        builder.RegisterService(&dPtr->pluginService);

        dPtr->server = builder.BuildAndStart();
        dPtr->address = addressUri.substr(0, addressUri.find_last_of(':'));
        LOG(INFO) << std::format(
            "Server listening on URI: {}, Port: {}", dPtr->address,
            dPtr->selectedPort
        );
    }
    dPtr->state = State::Running;
    return true;
}

bool Server::stop()
{
    std::unique_lock lock(dPtr->serverMtx);

    if (dPtr->state != State::Running)
        return false;

    dPtr->server->Shutdown();
    dPtr->state = State::Finished;
    return true;
}

bool Server::reset()
{
    std::unique_lock lock(dPtr->serverMtx);
    if (dPtr->state != State::Finished)
        return false;
    dPtr->server.reset();
    dPtr->address.clear();
    dPtr->selectedPort = -1;
    dPtr->state = State::Init;
    return true;
}

int Server::port() const noexcept
{
    return dPtr->selectedPort;
}

std::string_view Server::address() const noexcept
{
    return dPtr->address;
}

CLAP_RCI_END_NAMESPACE
