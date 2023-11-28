#include <core/blkringqueue.h>
#include <core/timestamp.h>

#include <catch2/catch_test_macros.hpp>

#include <latch>
#include <thread>

using namespace RCLAP_NAMESPACE;

TEST_CASE("BlkQueue" "[Core]") {
    SECTION("Enqueue and Dequeue") {
        BlkRingQueue<int> queue(3);

        REQUIRE(queue.tryEnqueue(1));
        REQUIRE(queue.tryEnqueue(2));
        REQUIRE(queue.tryEnqueue(3));
        REQUIRE(!queue.tryEnqueue(4));
        REQUIRE(queue.size() == 3);

        int item = -1;
        REQUIRE(queue.tryDequeue(item));
        REQUIRE(item == 1);
        REQUIRE(queue.tryDequeue(item));
        REQUIRE(item == 2);
        REQUIRE(queue.tryDequeue(item));
        REQUIRE(item == 3);
        REQUIRE(!queue.tryDequeue(item));
        REQUIRE(queue.size() == 0);
    }

    SECTION("Peek and Pop") {
        BlkRingQueue<std::string> queue(123);
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
        BlkRingQueue<int> queue(bufCapacity);

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
        using namespace std::chrono_literals;
        std::latch sync(1);
        BlkRingQueue<int> queue(1);
        const auto timeout = 10ms;
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
            REQUIRE(ts.elapsedMicros() >= timeout.count());
            REQUIRE(queue.tryEnqueue(1));
            sync.count_down();
        });
    }
}
