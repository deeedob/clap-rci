#include <core/logging.h>
#include <core/timestamp.h>
#include <plugin/coreplugin.h>
#include <server/serverctrl.h>
#include <server/tags/servereventstream.h>
#include <iostream>

#include <latch>
#include <thread>

using namespace QCLAP_NAMESPACE;

const clap_plugin_descriptor desc = {};
clap_host host;

class TestClient
{
public:
    std::latch latch;

    explicit TestClient(const std::shared_ptr<grpc::Channel> &channel, std::uint64_t hash)
        : latch(1), stub(ClapInterface::NewStub(channel)), hash(hash)
    { }
    void clientParamCall(uint32_t max, uint32_t msgSize)
    {
        ClientParams request;
        None response;
        auto populateData = [&] {
            request.Clear();
            // populate params
            for (uint32_t i = 0; i < msgSize; ++i) {
                auto *p = request.add_params();
                p->set_event(Event::ParamValue);
                p->mutable_param()->set_param_id(i);
                p->mutable_param()->set_value(static_cast<double>(i));
                p->mutable_param()->set_modulation(static_cast<double>(i) / 2);
                auto s = Timestamp::stamp();
                p->mutable_timestamp()->set_seconds(s.seconds());
                p->mutable_timestamp()->set_nanos(s.nanos());
            }
        };
        populateData();
        std::string shash = std::to_string(hash);

        latch.wait();
        Timestamp ts;
        uint32_t  i = 0;
        for (; i < max; ++i) {
            grpc::ClientContext ctx;
            ctx.AddMetadata(Metadata::PluginHashId.data(), shash);
            const auto status = stub->ClientParamCall(&ctx, request, &response);
            if (!status.ok()) {
                std::cerr << "ClientParamCall failed: " << status.error_code() << ": " << status.error_message() << std::endl;
                break;
            }
        }
        const auto elapsedMs = ts.elapsedMillis();
        const auto elapsedNs = ts.elapsedNanos();
        const auto totalMsgs = i * msgSize;
        //Print Benchmark information
        std::cout << "Client Param Call Benchmark results:" << std::endl;
        std::cout << "    - RPC Calls: " << i << std::endl;
        std::cout << "    - Total time: " << elapsedMs << " ms / " << elapsedNs << " ns" << std::endl;
        std::cout << "    - Total messages: " << totalMsgs << std::endl;
        std::cout << "    - Average time per message: " << elapsedNs / totalMsgs << " ns" << std::endl;
        std::cout << "    - Average time per message: " << static_cast<double>(elapsedNs / totalMsgs) / 1'000 << " us" << std::endl;
        std::cout << "    - Average time per message: " << static_cast<double>(elapsedNs / totalMsgs) / 1'000'000 << " ms" << std::endl;
    }

    bool serverEventStream(std::latch &hostLatch)
    {
        grpc::ClientContext ctx;
        ctx.AddMetadata(Metadata::PluginHashId.data(), std::to_string(hash));

        ClientRequest request;
        ServerEvents response;
        auto stream = stub->ServerEventStream(&ctx, request);

        hostLatch.count_down(); // notify parent that we're about to read
        uint32_t i = 0;
        Timestamp ts;

        while(stream->Read(&response))
            for (const auto &ev : response.events())
                ++i;
        const auto elapsedMs = ts.elapsedMillis();
        const auto elapsedNs = ts.elapsedNanos();

        const auto status = stream->Finish();
        if (!status.ok())
            std::cerr << "ServerEventStream failed: " << status.error_code() << ": " << status.error_message() << std::endl;

        std::cout << "ServerEventStream Benchmark results:" << std::endl;
        std::cout << "  - Total messages: " << i << std::endl;
        std::cout << "  - Total time: " << elapsedMs << " ms / " << elapsedNs << " ns" << std::endl;
        std::cout << "  - Average time per message: " << elapsedNs / i << " ns" << std::endl;
        std::cout << "  - Average time per message: " << static_cast<double>(elapsedNs / i) / 1'000 << " us" << std::endl;
        std::cout << "  - Average time per message: " << static_cast<double>(elapsedNs / i) / 1'000'000 << " ms" << std::endl;

        return true;
    }

private:
    std::unique_ptr<ClapInterface::Stub> stub;
    std::uint64_t hash;
};

int main()
{
    CorePlugin cp(&desc, &host);
    const auto idHash = ServerCtrl::instance().addPlugin(&cp);
    ServerCtrl::instance().start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "Begin benchmark" << std::endl;
    std::cout << "  - Server address: " << *ServerCtrl::instance().address() << std::endl;

    assert(idHash);
    assert(*idHash != 0);
    TestClient client(grpc::CreateChannel(
        *ServerCtrl::instance().address(),
        grpc::InsecureChannelCredentials()),
        *idHash
    );

    uint32_t maxIterations = 10000;
    uint32_t msgSize = 10;

    // ClientParamCall benchmark
    auto th = std::jthread([&] {
        client.clientParamCall(maxIterations, msgSize);
    });
    client.latch.count_down();
    th.join();
    std::cout << std::endl;

    // ServerEventStream benchmark
    std::latch hostLatch(1);
    auto th2 = std::jthread([&] {
        client.serverEventStream(hostLatch);
    });

    auto sd = ServerCtrl::instance().getSharedData(*idHash);
    assert(sd);
    while (sd->nStreams() <= 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    hostLatch.wait();
    Timestamp ts;
    for (uint32_t i = 0; i < maxIterations; ++i) {
        for (uint32_t  k = 0; k < msgSize; ++k)
            sd->pluginToClientsQueue().push({ Event::NoteOn, ClapEventNoteWrapper() });
    }
    const auto elapsedNs = ts.elapsedNanos();
    const auto totalMsgs = maxIterations * msgSize;
    assert(sd->stopPoll());
    th2.join();
    std::cout << "Server Side Event Processing took: " << elapsedNs << " ns " << std::endl;
    std::cout << "Server Side Event Processing took: " << static_cast<double>(elapsedNs) / 1'000 << " us" << std::endl;
    std::cout << "Throughput: " << static_cast<double>(totalMsgs) / (static_cast<double>(elapsedNs / 1'000)) << " msgs/us" << std::endl;
}