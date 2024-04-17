#include <clap-rci/coreplugin.h>
#include <clap-rci/descriptor.h>
#include <clap-rci/gen/rci.grpc.pb.h>
#include <clap-rci/registry.h>

#include <absl/log/log.h>
#include <catch2/catch_test_macros.hpp>
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <string_view>
#include <thread>

using namespace CLAP_RCI_NAMESPACE;
using namespace std::chrono_literals;

clap_host kHost {
    .clap_version = CLAP_VERSION,
    .host_data = nullptr,
    .name = "TestHost",
    .vendor = "TestHost GmbH",
    .url = "https://testhost.test",
    .version = "v17.5",
    .get_extension = [](const clap_host *h, const char *extId) -> const void * {
        (void)h;
        (void)extId;
        return nullptr;
    },
    .request_restart =
        [](const clap_host *h) {

        },
    .request_process =
        [](const clap_host *h) {

        },
    .request_callback =
        [](const clap_host *h) {

        }
};

class Plugin : public CorePlugin
{
    REGISTER;

public:
    explicit Plugin(const clap_host *host)
        : CorePlugin(descriptor(), host)
    {
    }
    static const Descriptor *descriptor() noexcept
    {
        static Descriptor desc(
            "clap.rci.one", "TestPluginOne", "vendorone", "0.0.1"
        );
        return &desc;
    }

private:
};

class Client
{
public:
    explicit Client(const std::shared_ptr<grpc::ChannelInterface> &channel)
        : mPlugStub(api::ClapPluginService::NewStub(channel))
    {
    }

    void startEventStream(uint64_t pluginId)
    {
        grpc::ClientContext ctx;
        ctx.AddMetadata("plugin_id", std::to_string(pluginId));
        const auto stream = mPlugStub->EventStream(&ctx);

        api::PluginEventMessage pluginMsg;
        while (stream->Read(&pluginMsg))
            ;
    }

private:
    std::unique_ptr<api::ClapPluginService::Stub> mPlugStub;
};

TEST_CASE("Client connection", "[coreplugin]")
{
    Registry::init("/path/to/clap.clap");
    Registry::create(&kHost, Plugin::descriptor()->id().data());
    const auto *corePlugin = Registry::Instances::instances()
                                 .begin()
                                 ->second.get();
    std::this_thread::sleep_for(100ms);

    constexpr size_t numClients = 4;
    std::vector<Client> clients;
    for (size_t i = 0; i < numClients; ++i) {
        clients.emplace_back(grpc::CreateChannel(
            "localhost:3000", grpc::InsecureChannelCredentials()
        ));
    }

    std::vector<std::jthread> clientThreads;
    for (size_t i = 0; i < numClients; ++i) {
        clientThreads.emplace_back(
            &Client::startEventStream, &clients[i], corePlugin->pluginId()
        );
    }

    std::this_thread::sleep_for(100ms);

    corePlugin->clapPlugin()->destroy(corePlugin->clapPlugin());

    for (auto &ct : clientThreads)
        ct.join();

    std::cout << "Finished\n";
}
