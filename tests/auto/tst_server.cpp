#include <catch2/catch_test_macros.hpp>
#include <clap-rci/server.h>
#include <thread>

using namespace CLAP_RCI_NAMESPACE;

TEST_CASE("Registry", "[registry]")
{
    Server s1;
    REQUIRE(s1.start());
    REQUIRE(!s1.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    REQUIRE(s1.stop());
    REQUIRE(!s1.stop());

    REQUIRE(s1.reset());
    REQUIRE(s1.start("localhost:33451"));
    REQUIRE(s1.address() == "localhost");
    REQUIRE(s1.port() == 33451);
    REQUIRE(!s1.reset());
    REQUIRE(s1.stop());
}
