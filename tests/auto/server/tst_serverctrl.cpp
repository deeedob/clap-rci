#include <core/logging.h>
#include <core/timestamp.h>
#include <plugin/coreplugin.h>
#include <server/serverctrl.h>
#include <server/tags/servereventstream.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <latch>
#include <thread>

using namespace QCLAP_NAMESPACE;

// Startup and shutdown the server before and after the test run.
class ServerCtrlListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const&) override {
        REQUIRE(ServerCtrl::instance().start());
        CHECK(ServerCtrl::instance().nPlugins() == 0);
        REQUIRE(ServerCtrl::instance().isRunning());
    }

    void testRunEnded(const Catch::TestRunStats &testRunStats) override {
        CHECK(ServerCtrl::instance().nPlugins() == 0);
        REQUIRE(ServerCtrl::instance().isRunning());
        REQUIRE(ServerCtrl::instance().stop());
        REQUIRE(!ServerCtrl::instance().isRunning());
    }
}; CATCH_REGISTER_LISTENER(ServerCtrlListener)


const clap_plugin_descriptor desc = {};
clap_host host;

static constexpr std::array<Event, 10> EvTstData = { Event::GuiShow,         Event::GuiCreate,
                                                     Event::GuiDestroy,      Event::GuiSetTransient,
                                                     Event::GuiHide,         Event::GuiHide,
                                                     Event::GuiSetTransient, Event::GuiDestroy,
                                                     Event::GuiCreate,       Event::GuiShow };
// Client used for testing. All test functions will be called in a separate thread.
class TestClient
{
public:
    std::latch latch;
    explicit TestClient(const std::shared_ptr<grpc::Channel> &channel, std::uint64_t hash)
            : latch(1), stub(ClapInterface::NewStub(channel)), hash(hash)
    {
    }
    // Client -> Server Blocking Queue
    bool clientEventCall(std::uint32_t max)
    {
        ClientEvent request;
        None response;

        latch.wait();
        for (std::uint32_t i = 0; i < max; ++i) {
            grpc::ClientContext ctx;
            request.set_event(EvTstData[i % EvTstData.size()]);
            ctx.AddMetadata(Metadata::PluginHashId.data(), std::to_string(hash));
            const auto status = stub->ClientEventCall(&ctx, request, &response);
            REQUIRE(status.ok());
        }
        return true;
    }
    // Client -> Server Param Queue
    bool clientParamCall(uint32_t max, uint32_t nparams)
    {
        ClientParams request;
        None response;
        auto populateData = [&] {
            request.Clear();
            // populate params
            for (uint32_t i = 0; i < nparams; ++i) {
                auto *p = request.add_params();
                p->set_event(Event::ParamGestureBegin);
                p->mutable_param()->set_param_id(i);
                p->mutable_param()->set_value(static_cast<double>(i));
                p->mutable_param()->set_modulation(static_cast<double>(i) / 2);
                auto s = Timestamp::stamp();
                p->mutable_timestamp()->set_seconds(s.seconds());
                p->mutable_timestamp()->set_nanos(s.nanos());
            }
        };
        latch.wait();
        for (std::uint32_t i = 0; i < max; ++i) {
            populateData();
            grpc::ClientContext ctx;
            ctx.AddMetadata(Metadata::PluginHashId.data(), std::to_string(hash));
            const auto status = stub->ClientParamCall(&ctx, request, &response);
            REQUIRE(status.ok());
        }

        return true;
    }

    bool serverEventStream(std::uint32_t max, std::latch &hostLatch)
    {
        grpc::ClientContext ctx;
        ctx.AddMetadata(Metadata::PluginHashId.data(), std::to_string(hash));

        ClientRequest request;
        ServerEvents response;
        auto stream = stub->ServerEventStream(&ctx, request);

        hostLatch.count_down(); // notify parent that we're about to read
        while(stream->Read(&response)) {
            for (const auto &ev : response.events()) {
                SPDLOG_INFO("Got event: {}", static_cast<int>(ev.event()));
            }
        }

        const auto status = stream->Finish();
        REQUIRE(status.ok());
        return true;
    }

private:
    std::unique_ptr<ClapInterface::Stub> stub;
    std::uint64_t hash;
};


//TEST_CASE("ServerCtrl")
//{
//    SECTION("Start/Stop")
//    {
//        REQUIRE(ServerCtrl::instance().start());
//        REQUIRE(!ServerCtrl::instance().start());
//        REQUIRE(ServerCtrl::instance().stop());
//        REQUIRE(!ServerCtrl::instance().stop());
//        REQUIRE(!ServerCtrl::instance().isRunning());
//    }
//
//    SECTION("Add/Remove Plugin")
//    {
//        CorePlugin cp(&desc, &host);
//        const auto idHash = ServerCtrl::instance().addPlugin(&cp);
//        REQUIRE(idHash);
//        REQUIRE(idHash != 0);
//        REQUIRE(ServerCtrl::instance().nPlugins() == 1);
//        REQUIRE(ServerCtrl::instance().removePlugin(*idHash));
//        REQUIRE(ServerCtrl::instance().nPlugins() == 0);
//        REQUIRE(!ServerCtrl::instance().removePlugin(*idHash));
//
//        CorePlugin cp2(&desc, &host);
//        CorePlugin cp3(&desc, &host);
//        const auto idHash2 = ServerCtrl::instance().addPlugin(&cp2);
//        const auto idHash3 = ServerCtrl::instance().addPlugin(&cp3);
//        REQUIRE(idHash2);
//        REQUIRE(idHash2 != 0);
//        REQUIRE(idHash3);
//        REQUIRE(idHash3 != 0);
//        REQUIRE(ServerCtrl::instance().nPlugins() == 2);
//        REQUIRE(ServerCtrl::instance().removePlugin(*idHash2));
//        REQUIRE(ServerCtrl::instance().nPlugins() == 1);
//        REQUIRE(ServerCtrl::instance().removePlugin(*idHash3));
//        REQUIRE(ServerCtrl::instance().nPlugins() == 0);
//    }
//}

TEST_CASE("Client Tags")
{
//    SECTION("ClientEventCall")
//    {
//        REQUIRE(ServerCtrl::instance().isRunning());
//
//        CorePlugin cp(&desc, &host);
//        const auto idHash = ServerCtrl::instance().addPlugin(&cp);
//        REQUIRE(idHash);
//        REQUIRE(*idHash != 0);
//
//        TestClient client(
//            grpc::CreateChannel(
//                *ServerCtrl::instance().address(), grpc::InsecureChannelCredentials()
//            ),
//            *idHash
//        );
//
//        std::uint32_t max = 25;
//        auto c = std::jthread([&]() { client.clientEventCall(max); });
//
//        auto sd = ServerCtrl::instance().getSharedData(*idHash);
//
//        client.latch.count_down();
//        for (std::uint32_t i = 0; i < max; ++i) {
//            REQUIRE(sd->blockingVerifyEvent(EvTstData[i % EvTstData.size()]));
//        }
//
//        REQUIRE(ServerCtrl::instance().removePlugin(*idHash));
//    }
//
//    SECTION("ClientParamCall") {
//        REQUIRE(ServerCtrl::instance().isRunning());
//        CHECK(ServerCtrl::instance().nPlugins() == 0);
//
//        CorePlugin cp(&desc, &host);
//        const auto idHash = ServerCtrl::instance().addPlugin(&cp);
//        REQUIRE(idHash);
//        REQUIRE(*idHash != 0);
//
//        TestClient client(
//            grpc::CreateChannel(*ServerCtrl::instance().address(),
//            grpc::InsecureChannelCredentials()),
//            *idHash
//        );
//
//        uint32_t iter = 10;
//        uint32_t nparams = 128;
//        auto c = std::jthread([&]() {
//            REQUIRE(client.clientParamCall(iter, nparams));
//        });
//
//        auto sd = ServerCtrl::instance().getSharedData(*idHash);
//        client.latch.count_down();
//        ClientParamWrapper ce;
//        for (uint32_t i = 0; i < iter * nparams;) {
//            // Read params from queue. pop() will return false if the queue is empty.
//            if (sd->clientsToPlugin().pop(ce)) {
//                const auto v = static_cast<uint32_t>(i % nparams);
//                CHECK(ce.value == static_cast<double>(v));
//                CHECK(ce.paramId == v);
//                CHECK(ce.ev == Event::ParamGestureBegin);
//                ++i;
////                SPDLOG_INFO("Got param: {} {}",ce.paramId, ce.value);
//            } else {
//                SPDLOG_TRACE("Client Param Call: WAIT");
//                std::this_thread::sleep_for(std::chrono::milliseconds(1));
//            }
//        }
//
//        c.join();
//        REQUIRE(ServerCtrl::instance().removePlugin(*idHash));
//    }

    SECTION("ServerEventStream") {
        // this thread
        CorePlugin cp(&desc, &host);
        const auto idHash = ServerCtrl::instance().addPlugin(&cp);

        REQUIRE(idHash);
        REQUIRE(*idHash != 0);
        TestClient client(grpc::CreateChannel(
            *ServerCtrl::instance().address(),
            grpc::InsecureChannelCredentials()),
            *idHash
        );

        // client in other thread
        std::latch hostLatch(1);
        uint32_t iter = 1;
        auto c = std::jthread([&]() {
            REQUIRE(client.serverEventStream(iter, hostLatch));
        });

        // We need to wait for the client to connect to the server and verify that.
        hostLatch.wait();
        auto sd = ServerCtrl::instance().getSharedData(*idHash);
        CHECK(sd != nullptr);
        while (sd->nStreams() <= 0) {
            SPDLOG_INFO("ServerEventStream: WAIT for stream");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        for (uint32_t  n = 0; n < iter; ++n) {
            for (uint32_t i = 0; i < 16; ++i) {
                sd->pluginToClientsQueue().push({Event::NoteOn, ClapEventNoteWrapper() });
                sd->pluginToClientsQueue().push({Event::NoteOff, ClapEventNoteWrapper() });
                sd->pluginToClientsQueue().push({Event::ParamGestureBegin, ClapEventParamWrapper() });
                sd->pluginToClientsQueue().push({Event::ParamGestureEnd, ClapEventParamWrapper() });
                sd->pluginToClientsQueue().push({Event::GuiSetTransient, ClapEventMainSyncWrapper() });
                sd->pluginToClientsQueue().push({Event::GuiShow, ClapEventMainSyncWrapper() });
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        sd->stopPoll();
        c.join();

        REQUIRE(ServerCtrl::instance().removePlugin(*idHash));
    }
}
