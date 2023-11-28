#include <server/server.h>

#include <catch2/catch_test_macros.hpp>

using namespace RCLAP_NAMESPACE;

TEST_CASE("Server Test") {
    SECTION("Start/Stop") {
        Server server;
        REQUIRE(server.currentState() == Server::State::CREATE);
        CHECK(!server.port());
        CHECK(server.uri() == "0.0.0.0");
        REQUIRE(!server.address());

        REQUIRE(server.start());
        REQUIRE(server.port() != -1);
        REQUIRE(*server.address() == "0.0.0.0:" + std::to_string(*server.port()));
        REQUIRE(server.currentState() == Server::State::RUNNING);
        REQUIRE(!server.start());

        REQUIRE(server.stop());
        REQUIRE(server.currentState() == Server::State::FINISHED);
        REQUIRE(!server.stop());

        REQUIRE(!server.start());
        REQUIRE(!server.stop());
    }
    SECTION("Custom Port") {
        Server server("localhost:5056");
        CHECK(!server.port());
        REQUIRE(server.start());
        REQUIRE(server.port() == 5056);
        REQUIRE(*server.address() == "localhost:5056");
        REQUIRE(server.stop());
    }
}
