#include <core/processhandle.h>
#include <core/timestamp.h>

#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <string_view>

using namespace QCLAP_NAMESPACE;

#if defined _WIN32 || defined _WIN64
constexpr std::string_view path = "executable.exe";
#elif defined __linux__ || defined __APPLE__
constexpr std::string_view path = "executable";
#endif


void checkTeardown(ProcessHandle &handle)
{
    REQUIRE(!handle.checkChildStatus());
    REQUIRE(!handle.isChildRunning());
    REQUIRE(!handle.terminateChild());
}

TEST_CASE("ProcessHandle" "[Core]") {

    SECTION("Basic") {
        ProcessHandle handle;
        REQUIRE(!handle);
        REQUIRE(!handle.startChild());
        REQUIRE(!handle.terminateChild());

        REQUIRE(!handle.setExecutable("executablee"));
        REQUIRE(handle.setExecutable(path));
        REQUIRE(handle);
        REQUIRE(handle.setArguments({ "1", "2", "3" }));

        REQUIRE(handle.getPath().string() == path.data());
        REQUIRE(handle.getArguments()[0] == "1");
        REQUIRE(handle.getArguments()[1] == "2");
        REQUIRE(handle.getArguments()[2] == "3");

        handle.clearArguments();
        REQUIRE(handle.getArguments().empty());
        REQUIRE(handle.startChild());
        const auto status = handle.waitForChild();
        REQUIRE(status);
        REQUIRE(*status == 126);
        checkTeardown(handle);
    }

    SECTION("With Timeout") {
        long long timeout = 200;
        Timestamp ts;
        ProcessHandle handle(path.data(), { std::to_string(timeout) });
        REQUIRE(handle);
        REQUIRE(handle.startChild());
        const auto status = handle.waitForChild();
        REQUIRE(ts.elapsedMillis() >= timeout);
        REQUIRE(status);
        REQUIRE(*status == 254);
        checkTeardown(handle);
    }

    SECTION("With Kill") {
        long long timeout = 15;
        ProcessHandle handle(path.data(), { "1", "2" });
        REQUIRE(handle);
        REQUIRE(handle.startChild());

        Timestamp ts;
        const auto iter = 3;
        const auto t = timeout / iter;
        for (int i = 0; i < iter; ++i) {
            REQUIRE(!handle.checkChildStatus());
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        }

        const auto res = handle.terminateChild();
        REQUIRE(res);
        REQUIRE(ts.elapsedMillis() >= timeout);
// TODO: improve this and make processhandle more robust
#if defined _WIN32 || defined _WIN64
        REQUIRE(*res == 0);
#elif defined __linux__ || defined __APPLE__
        REQUIRE(*res == SIGTERM);
#endif
        checkTeardown(handle);
    }
}
