#include <core/processhandle.h>
#include <core/timestamp.h>
#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <string_view>

using namespace QCLAP_NAMESPACE;

constexpr std::string_view path = "executable";


void checkTeardown(ProcessHandle &handle)
{
    REQUIRE(!handle.checkChildBlocking());
    REQUIRE(!handle.childRunning());
    REQUIRE(!handle.killChildAndWait());
    REQUIRE(!handle.killChild());
    REQUIRE(!handle.checkChild());
}

TEST_CASE("ProcessHandle" "[Core]") {

    SECTION("Basic") {
        ProcessHandle handle;
        REQUIRE(!handle);
        REQUIRE(!handle.execute());
        REQUIRE(!handle.killChild());

        REQUIRE(!handle.setExecutable("executablee"));
        REQUIRE(handle.setExecutable(path));
        REQUIRE(handle);
        REQUIRE(handle.setArgs({"1", "2", "3"}));

        REQUIRE(handle.path().string() == path.data());
        REQUIRE(handle.args()[0] == "1");
        REQUIRE(handle.args()[1] == "2");
        REQUIRE(handle.args()[2] == "3");

        handle.clearArgs();
        REQUIRE(handle.args().empty());
        REQUIRE(handle.execute());
        const auto status = handle.checkChildBlocking();
        REQUIRE(status);
        REQUIRE(*status == 126);
        checkTeardown(handle);
    }

    SECTION("With Timeout") {
        long long timeout = 200;
        Timestamp ts;
        ProcessHandle handle(path.data(), { std::to_string(timeout) });
        REQUIRE(handle);
        REQUIRE(handle.execute());
        const auto status = handle.checkChildBlocking();
        REQUIRE(ts.elapsedMillis() >= timeout);
        REQUIRE(status);
        REQUIRE(*status == 254);
        checkTeardown(handle);
    }

    SECTION("With Kill") {
        long long timeout = 15;
        ProcessHandle handle(path.data(), { "1", "2" });
        REQUIRE(handle);
        REQUIRE(handle.execute());

        Timestamp ts;
        const auto iter = 3;
        const auto t = timeout / iter;
        for (int i = 0; i < iter; ++i) {
            REQUIRE(!handle.checkChild());
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        }
        REQUIRE(handle.killChild());
        REQUIRE(ts.elapsedMillis() >= timeout);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        const auto status = handle.checkChild();
        REQUIRE(status);
        REQUIRE(*status == 15);
        checkTeardown(handle);
    }
}
