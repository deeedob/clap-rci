#include <clap-rci/mpmcqueue.h>
#include <clap-rci/gen/rci.pb.h>

#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <thread>
#include <format>

using namespace CLAP_RCI_NAMESPACE;
using namespace CLAP_RCI_API_VERSION;

TEST_CASE("MpMcQueue Basic")
{
    MpMcQueue<int, 4> queue;
    REQUIRE(queue.tryPush(1));
    REQUIRE(queue.size() == 1);
    REQUIRE(queue.tryPush(2));
    REQUIRE(queue.size() == 2);
    REQUIRE(queue.tryPush(3));
    REQUIRE(queue.size() == 3);
    REQUIRE(queue.tryPush(4));
    REQUIRE(queue.size() == 4);
    int dq = 0;
    REQUIRE(queue.pop(&dq));
    REQUIRE(queue.size() == 3);
    REQUIRE(dq == 1);
    REQUIRE(queue.pop(&dq));
    REQUIRE(queue.size() == 2);
    REQUIRE(dq == 2);
    REQUIRE(queue.pop(&dq));
    REQUIRE(queue.size() == 1);
    REQUIRE(dq == 3);
    REQUIRE(queue.pop(&dq));
    REQUIRE(dq == 4);
    REQUIRE(queue.size() == 0);
    REQUIRE(queue.isEmpty());
}

TEST_CASE("MpMcQueue push")
{
    MpMcQueue<int, 4> queue;
    REQUIRE(queue.push(1));
    REQUIRE(queue.push(2));
    REQUIRE(queue.push(3));
    REQUIRE(queue.push(4));
    REQUIRE(queue.push(5));
    REQUIRE(queue.push(6));

    int dq = 0;
    REQUIRE(queue.pop(&dq));
    REQUIRE(dq == 3);
    REQUIRE(queue.pop(&dq));
    REQUIRE(dq == 4);
    REQUIRE(queue.pop(&dq));
    REQUIRE(dq == 5);
    REQUIRE(queue.pop(&dq));
    REQUIRE(dq == 6);
}

TEST_CASE("MpMcQueue Threaded")
{
    MpMcQueue<int, 1024> stack;

    std::jthread tprod([&stack](){
        int temp = 0;
        int overwrites = 0;
        for (int i = 0; i < 1'000'000; ++i) {
            if (!stack.tryPush(i)) {
                stack.pop(&temp);
                ++overwrites;
            }
        }
        std::cout << std::format("overwrites: {}\n", overwrites);
    });

    std::jthread tconsumer([&stack](std::stop_token stoken){
        int next = 0;
        int current = 0;

        int success = 0;
        int misses = 0;
        while (!stoken.stop_requested()) {
            if (!stack.pop(&next)) {
                ++misses;
            } else {
                REQUIRE(current <= next);
                current = next;
                ++success;
            }
        }
        std::cout << std::format("success: {}, misses: {}\n", success, misses);
    });

    tprod.join();
    tconsumer.request_stop();
    tconsumer.join();
}
