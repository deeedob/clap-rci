#include <core/reducequeue.h>
#include <server/shareddata.h>
#include <catch2/catch_test_macros.hpp>

using namespace QCLAP_NAMESPACE;
namespace capi = api::v0;

TEST_CASE("ReduceQueue" "[Core]") {
    SECTION("Basic") {
        size_t max = 100;
        ReduceQueue<clap_id, PluginParam> queue;
        auto setup = [&](size_t max) {
            queue.setCapacity(max);
            for (size_t i = 0; i <= max; ++i) {
                queue.set(clap_id(i), { static_cast<double>(i), static_cast<double>(i) });
            }
            queue.producerDone();
        };
        setup(max);

        size_t count = max;
        queue.consume([&](clap_id id, PluginParam p) -> void {
            REQUIRE(id == count);
            auto d = static_cast<double>(count);
            REQUIRE(p == PluginParam{d, d});
            --count;
        });
    }
    SECTION("Overwrite") {
        ReduceQueue<clap_id, PluginParam> queue;
        queue.set(clap_id(0), { 0.0, 0.1 });
        queue.set(clap_id(1), { 1.0, 1.0 });
        queue.set(clap_id(1), { 1.0, 1.1 });

        queue.producerDone();

        size_t count = 1;
        queue.consume([&](clap_id id, PluginParam p) -> void {
            REQUIRE(id == count);
            auto pp = static_cast<double>(count);
            auto pm = static_cast<double>(count) + 0.1;
            REQUIRE(p == PluginParam{pp, pm});
            --count;
        });
    }
    SECTION("Update") {
        ReduceQueue<clap_id, ClientParamWrapper> queue;

        auto create = [] (clap_id id, ClientParam::Type type, double value, int64_t s, int64_t n) -> ClientParam {
            ClientParam p;
            p.set_param_id(id);
            p.set_type(type);
            p.set_value(value);
            p.mutable_timestamp()->set_seconds(s);
            p.mutable_timestamp()->set_nanos(n);
            return p;
        };

        // We update by timestamp
        ClientParam cp0 = create(0, capi::ClientParam_Type_Begin, 0.0, 0, 0);
        ClientParam cp1 = create(1, capi::ClientParam_Type_Begin, 0.5, 1, 2);
        ClientParam cp2 = create(1, capi::ClientParam_Type_Begin, 1.0, 1, 1); // smallest one
        ClientParam cp3 = create(1, capi::ClientParam_Type_Begin, 1.0, 1, 3);

        queue.setOrUpdate(cp0.param_id(), cp0);
        queue.setOrUpdate(cp1.param_id(), cp1);
        queue.setOrUpdate(cp2.param_id(), cp2);
        queue.setOrUpdate(cp3.param_id(), cp3);

        size_t count = 1;
        queue.consume([&](clap_id id, ClientParamWrapper p) {
            REQUIRE(id == count);
            REQUIRE(p.type == capi::ClientParam_Type_Begin);
            REQUIRE(p.value == static_cast<double>(count));
            REQUIRE(p.stamp.seconds() == static_cast<int64_t>(count));
            REQUIRE(p.stamp.nanos() == static_cast<int64_t>(count));
            --count;
        });
    }
}