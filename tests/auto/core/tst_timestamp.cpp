#include <core/timestamp.h>
#include <core/logging.h>
#include <catch2/catch_test_macros.hpp>
#include <thread>

void sleepMs(std::uint32_t t)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(t));
}

using namespace QCLAP_NAMESPACE;

TEST_CASE("Timestamp" "[Core]") {

    SECTION("Elapsed Time in Milliseconds") {
        Timestamp ts;
        sleepMs(100);
        REQUIRE(ts.elapsedMillis() >= 100);
    }

    SECTION("Elapsed Time in Nanoseconds") {
        Timestamp ts;
        sleepMs(100);
        REQUIRE(ts.elapsedNanos() >= 100000000);
    }

    SECTION("Formatted Elapsed Time") {
        Timestamp ts;
        sleepMs(100);
        std::string msElapsed = ts.fmtElapsed(Timestamp::Type::MS);
        std::string nsElapsed = ts.fmtElapsed(Timestamp::Type::NS);

        // Ensure that the formatted times contain "ms" and "ns" respectively
        REQUIRE(msElapsed.find("ms") != std::string::npos);
        REQUIRE(nsElapsed.find("ns") != std::string::npos);

        // Parse the numeric part and check if it's greater than or equal to 100
        long long msValue = std::stoll(msElapsed);
        long long nsValue = std::stoll(nsElapsed);
        REQUIRE(msValue >= 100);
        REQUIRE(nsValue >= 100000000);
    }

    SECTION("Formatted Timestamp") {
        Timestamp ts;
        std::string msTimestamp = ts.fmtStamp(Timestamp::Type::MS);
        std::string nsTimestamp = ts.fmtStamp(Timestamp::Type::NS);

        // Ensure that the formatted timestamps contain "ms" and "ns" respectively
        REQUIRE(msTimestamp.find("ms") != std::string::npos);
        REQUIRE(nsTimestamp.find("ns") != std::string::npos);

        // Parse the numeric part and check if it's not zero
        long long msValue = std::stoll(msTimestamp);
        long long nsValue = std::stoll(nsTimestamp);
        REQUIRE(msValue != 0);
        REQUIRE(nsValue != 0);
    }

    SECTION("Ticks") {
        std::uint64_t ticks = Timestamp::getFastTicks();
        REQUIRE(ticks > 0);
    }

    SECTION("Stamp") {
        Stamp st;
        CHECK(st.seconds() == 0);
        CHECK(st.nanos() == 0);

        uint32_t max = 10;
        for (uint32_t i = 0; i < max; ++i) {
            auto st1 = Timestamp::stamp();
            Stamp st2 = Timestamp::stamp();
            REQUIRE(st1 < st2);
            REQUIRE(st2 > st1);
        }
    }
}
