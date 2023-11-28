#include <core/logging.h>
#include <server/server.h>
#include <server/cqeventhandler.h>
#include <iostream>

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

using namespace RCLAP_NAMESPACE;

TEST_CASE("CqEventHandler") {

    SECTION("Callback Benchmarks") {
        Server s;
        REQUIRE(s.start());
        CqEventHandler *streamCq = nullptr;
        while(!streamCq) {
            streamCq = s.getServerStreamCqHandle();
            SPDLOG_INFO("Waiting for streamCq");
        }
//        BENCHMARK("Callback") {
            std::array<float, 32> buffer = {0.0f};
            for (int i = 0; i < 700; ++i) {
                streamCq->enqueueFn([&buffer](bool ok) {
                    if (ok) {
                        for (auto &v : buffer)
                            v += 1.36f;
                    }}
                , 200000);
            }
//        };
        while (streamCq->hasPendingAlarms()) {
           SPDLOG_INFO("Waiting for streamCq to finish");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            streamCq->cancelAllPendingTags();
        }
        for (const auto &v : buffer)
            std::cout << v << " ";
        std::cout << std::endl;
        REQUIRE(s.stop());
    }
}
