#include <core/blkqueue.h>
#include <core/timestamp.h>

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <latch>
#include <thread>
#include <memory>

using namespace QCLAP_NAMESPACE;

TEST_CASE("Semaphore" "[Core]") {
    SECTION("bool wait()") {
        auto sem = std::make_unique<Semaphore>();
        Timestamp ts;
        REQUIRE(sem->value() == 0);

        std::jthread tj([&]() {
            REQUIRE(sem->wait());
            REQUIRE(ts.elapsedMillis() >= 2);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        sem->signal();
        tj.join();
        sem.reset();

        ts.reset();
        sem = std::make_unique<Semaphore>(2);
        REQUIRE(sem->value() == 2);
        tj = std::jthread([&]() {
            REQUIRE(sem->wait());
            REQUIRE(sem->value() == 2);
            REQUIRE(sem->wait());
            REQUIRE(sem->value() == 1);
        });
        sem->signal();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        sem->signal();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        tj.join();
        REQUIRE(sem->value() == 2);
    }

    SECTION("Producer/Consumer") {
        const int bufCapacity = 5;
        const int nItems = 10;
        std::vector<int> buffer(bufCapacity);

        Semaphore emptySlot(bufCapacity);
        Semaphore fullSlot(0);
        Semaphore mutex(1);

        std::jthread producer([&]() {
            for (int i = 0; i < nItems; ++i) {
                int item = i + 10;

                emptySlot.wait();
                mutex.wait();
                const auto idx = static_cast<std::size_t>(i % bufCapacity);
                buffer[idx] = item;

                mutex.signal();
                fullSlot.signal();
            }
        });

        std::jthread consumer([&]() {
            for (int i = 0; i < nItems; ++i) {
                fullSlot.wait();
                mutex.wait();

                const auto idx = static_cast<std::size_t>(i % bufCapacity);
                int item = buffer[idx];
                REQUIRE(item == i + 10);

                mutex.signal();
                emptySlot.signal();
            }
        });
    }
}

TEST_CASE("BlkQueue" "[Core]") {
    SECTION("Enqueue and Dequeue") {
        BlkQueue<int> queue(3);

        REQUIRE(queue.tryEnqueue(1));
        REQUIRE(queue.tryEnqueue(2));
        REQUIRE(queue.tryEnqueue(3));

        int item = -1;
        REQUIRE(queue.tryDequeue(item));
        REQUIRE(item == 1);
        REQUIRE(queue.tryDequeue(item));
        REQUIRE(item == 2);
        REQUIRE(queue.tryDequeue(item));
        REQUIRE(item == 3);
    }

    SECTION("Peek and Pop") {
        BlkQueue<std::string> queue(123);
        REQUIRE(queue.tryEnqueue("one"));
        REQUIRE(queue.tryEnqueue("two"));
        REQUIRE(queue.tryEnqueue("three"));

        std::string item;
        REQUIRE(*queue.peek() == "one");
        REQUIRE(queue.tryPop());
        REQUIRE(*queue.peek() == "two");
        REQUIRE(queue.tryPop());
        REQUIRE(*queue.peek() == "three");
        REQUIRE(queue.tryPop());
    }

    SECTION("Producer/Consumer") {
        const int bufCapacity = 5;
        const int nItems = 10;
        BlkQueue<int> queue(bufCapacity);

        std::jthread producer([&]() {
            for (int i = 0; i < nItems; ++i) {
                int item = i + 10;

                while (!queue.tryEnqueue(item)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });

        std::jthread consumer([&]() {
            for (int i = 0; i < nItems; ++i) {
                int item = -1;
                while (!queue.tryDequeue(item)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                REQUIRE(item == i + 10);
            }
        });
    }

    SECTION("Timeout") {
        std::latch sync(1);
        BlkQueue<int> queue(1);
        std::int64_t timeout = 10'000; // 10ms
        Timestamp ts;

        std::jthread consumer([&](){
            int item = -1;
            REQUIRE(!queue.waitDequeueTimed(item, timeout));
            sync.wait();
            REQUIRE(queue.waitDequeueTimed(item, timeout));
            REQUIRE(item == 1);
            REQUIRE(queue.peek() == nullptr);
        });

        std::jthread producer([&](){
            std::this_thread::sleep_for(std::chrono::microseconds(2 * timeout));
            REQUIRE(ts.elapsedMicros() >= timeout);
            REQUIRE(queue.tryEnqueue(1));
            sync.count_down();
        });
    }

    SECTION("Chrono timeout") {
        BlkQueue<int> queue(1);
        std::chrono::milliseconds timeout(10);
        std::chrono::milliseconds initTimeout{10'000};
    }
}
