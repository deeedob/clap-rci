#include <clap-rci/gen/rci.pb.h>
#include <clap-rci/mpmcqueue.h>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <format>
#include <iostream>
#include <thread>

using namespace CLAP_RCI_NAMESPACE;
using namespace CLAP_RCI_API_VERSION;

TEST_CASE("tryPush", "[MpMcQueue]")
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

TEST_CASE("push", "[MpMcQueue]")
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

TEST_CASE("Single threaded", "[MpMcQueue]")
{
    MpMcQueue<int, 1024> queue;

    std::jthread tprod([&queue]() {
        int temp = 0;
        int overwrites = 0;
        for (int i = 0; i < 1'000'000; ++i) {
            if (!queue.tryPush(i)) {
                queue.pop(&temp);
                ++overwrites;
            }
        }
        std::cout << std::format("overwrites: {}\n", overwrites);
    });

    std::jthread tconsumer([&queue](std::stop_token stoken) {
        int next = 0;
        int current = 0;

        int success = 0;
        int misses = 0;
        while (!stoken.stop_requested()) {
            if (!queue.pop(&next)) {
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

TEST_CASE("Multi threaded", "[MppMcQueue]")
{
    constexpr int nProducers = 16;
    constexpr int nConsumers = 16;
    constexpr size_t nProdIter = 1'000'000;

    MpMcQueue<size_t, 1024> queue;

    std::atomic<long> producerMisses {0};
    std::atomic<long> producerWrites {0};
    std::vector<std::jthread> producers;
    for (size_t i = 0; i < nProducers; ++i) {
        producers.emplace_back([&queue, &producerMisses, &producerWrites, i]() {
            for (size_t j = 0; j < nProdIter; ++j) {
                if (!queue.push(i + j))
                    ++producerMisses;
                else
                    ++producerWrites;
            }
        });
    }

    std::atomic<long> consumerMisses {0};
    std::atomic<long> consumerReads {0};
    std::vector<std::jthread> consumers;
    for (size_t i = 0; i < nConsumers; ++i) {
        consumers.emplace_back([&queue, &consumerMisses,
                                &consumerReads](std::stop_token stoken) {
            size_t next {};
            while (!stoken.stop_requested()) {
                if (!queue.pop(&next))
                    ++consumerMisses;
                else
                    ++consumerReads;
            }
        });
    }

    for (auto &prod : producers)
        prod.join();
    for (auto &cons : consumers) {
        cons.request_stop();
        cons.join();
    }

    std::cout << std::format(
        "Producer writes: {}, misses: {}, ratio: {:.2f}\n",
        producerWrites.load(), producerMisses.load(),
        static_cast<double>(producerMisses)
            / static_cast<double>(producerWrites)
    );
    std::cout << std::format(
        "Consumer reads: {}, misses: {}, ratio: {:.2f}\n", consumerReads.load(),
        consumerMisses.load(),
        static_cast<double>(consumerMisses) / static_cast<double>(consumerReads)
    );
}

TEST_CASE("Queue Benchmarks", "[MpMcQueue]")
{
    {
        MpMcQueue<size_t, 1024> queue;
        size_t value = 0;
        const auto size = queue.size();
        BENCHMARK("Fitting push 1024")
        {
            for (size_t j = 0; j < size; ++j)
                queue.push(j);
            while (queue.pop(&value))
                ;
        };
    }
    {
        MpMcQueue<size_t, 1024> queue;
        size_t value = 0;
        const auto size = queue.size();
        BENCHMARK("Fitting tryPush 1024")
        {
            for (size_t j = 0; j < size; ++j)
                queue.tryPush(j);
            while (queue.pop(&value))
                ;
        };
    }
    {
        MpMcQueue<size_t, 1024> queue;
        size_t value = 0;
        const auto size = 2 * queue.size();
        BENCHMARK("Overwrite 2048")
        {
            for (size_t j = 0; j < size; ++j)
                queue.push(j);
            while (queue.pop(&value))
                ;
        };
    }
}
