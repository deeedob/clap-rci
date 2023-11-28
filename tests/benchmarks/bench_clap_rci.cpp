#include <core/logging.h>
#include <core/timestamp.h>
#include <core/processhandle.h>
#include <plugin/coreplugin.h>
#include <server/serverctrl.h>
#include <crill/progressive_backoff_wait.h>

using namespace RCLAP_NAMESPACE;
const clap_plugin_descriptor Desc = {};
clap_host Host;

static clap_event_note HostEvent = {
   .header = {
       .size = sizeof(sizeof(clap_event_note)),
       .time = 0,
       .space_id = CLAP_CORE_EVENT_SPACE_ID,
       .type = CLAP_EVENT_NOTE_ON,
       .flags = CLAP_EVENT_DONT_RECORD
   },
   .note_id = 13,
   .port_index = 1,
   .channel = 2,
   .key = 61,
   .velocity = 0.5
};

static ServerEventWrapper TestEvent = {
    Event::Note, ClapEventNoteWrapper(&HostEvent, CLAP_EVENT_NOTE_ON )
};

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <iterations> <evs/call>" << std::endl;
        return 1;
    }
    uint64_t iterations = std::stoull(argv[1]);
    uint64_t eventsPerIteration = std::stoull(argv[2]);
    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "Evs/Call: " << eventsPerIteration << std::endl;


    Log::setupLogger("");
    CorePlugin cp(&Desc, &Host);
    const auto idHash = *ServerCtrl::instance().addPlugin(&cp);
    auto sharedData = ServerCtrl::instance().getSharedData(idHash);
    ServerCtrl::instance().start();

    auto clockedEvent = [&](){
        crill::progressive_backoff_wait([&] {
            ServerEventWrapper clock({ Event::Param, ClapEventParamWrapper() });
            clock.ts = Timestamp::stamp();
            return sharedData->pluginToClientsQueue().push(std::move(clock));
        });
    };

    // Start Client
    const auto address = *ServerCtrl::instance().address();
    ProcessHandle child("clients/client-cpp", { std::to_string(idHash), address });

    SPDLOG_INFO("Begin Benchmarks");

    child.startChild();
    while (sharedData->nStreams() <= 0) ; // Busy wait for client to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    clockedEvent();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    for (uint64_t i = 0; i < iterations; ++i) {
        for (uint64_t k = 0; k < eventsPerIteration; ++k) {
            while (!sharedData->pluginToClientsQueue().push(ServerEventWrapper(TestEvent))) ;
        }
    }
    clockedEvent();

    std::this_thread::sleep_for(std::chrono::milliseconds(700));
    SPDLOG_INFO("End Benchmarks");
    sharedData->endStreams();

    child.waitForChild();

    return 0;
}
