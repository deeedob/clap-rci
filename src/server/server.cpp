#include <core/logging.h>
#include "server.h"
#include "tags/clienteventcall.h"
#include "tags/clientparamcall.h"
#include "tags/servereventstream.h"
#include <crill/progressive_backoff_wait.h>

RCLAP_BEGIN_NAMESPACE

Server::Server(std::string_view address)
    : serverAddress(address)
{
    if (!Log::Private::gLoggerSetup)
        Log::setupLogger("mServer");
}

Server::~Server()
{
    if (isRunning())
        stop();
}

bool Server::start()
{
    // Server can be started only once
    if (state != CREATE)
        return false;
    state = RUNNING;

    // Specify the amount of cqs and threads to use
    cqHandlers.reserve(2);
    threads.reserve(cqHandlers.capacity());
    // Create the server and completion queues. Launch server
    {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials(), &mActivePort);
        builder.RegisterService(&aservice);

        // Create the amount of completion queues
        CqEventHandler temp1(this, builder.AddCompletionQueue());
        cqHandlers.emplace_back(std::make_unique<CqEventHandler>(std::move(temp1)));
        CqEventHandler temp2(this, builder.AddCompletionQueue());
        cqHandlers.emplace_back(std::make_unique<CqEventHandler>(std::move(temp2)));
        server = builder.BuildAndStart();
        SPDLOG_INFO("Server listening on URI: {}, Port: {}", uri(), mActivePort);
    }

    // Create handlers to manage the RPCs and distribute them across
    // the completion queues. We use the first completion queue for
    // server-side streaming from the plugin to its client.
    cqHandlers[PosStreamCq]->create<ServerEventStream>();
    SPDLOG_TRACE("Server-Stream completion queue served @ {}", toTag(cqHandlers[PosStreamCq].get()));
    cqHandlers[1]->create<ClientEventCallHandler>();
    cqHandlers[1]->create<ClientParamCall>();

    // Distribute completion queues across threads
    threads.emplace_back(&CqEventHandler::run, cqHandlers[PosStreamCq].get());
    threads.emplace_back(&CqEventHandler::run, cqHandlers[1].get());

    return true;
}

bool Server::stop()
{
    if (!isRunning())
        return false;

    // Shutdown the server and all completion queues. This drains
    // all the pending events and allows the threads to exit gracefully.
    for (const auto & c : cqHandlers)
        c->cancelAllPendingTags();
    server->Shutdown();         // shutdown server
    for (const auto &c : cqHandlers)
        c->teardown();          // teardown the completion queues, drain all pending events
    for (auto &c : cqHandlers) {
         crill::progressive_backoff_wait([&c]{          // sync all threads. Wait for leftovers
            return c->getState() == CqEventHandler::SHUTDOWN;
        });
    }

    // Wait for all threads to exit
    for (auto &j : threads) // commit
        j.join();

    state = FINISHED;
    return true;
}

void Server::wait(std::uint32_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

RCLAP_END_NAMESPACE
