#include <iostream>
#include <memory>
#include <chrono>

#include <core/global.h>
#include <core/timestamp.h>

#include <api.pb.h>
#include <api.grpc.pb.h>
#include <grpcpp/grpcpp.h>

using namespace api::v0;
using namespace RCLAP_NAMESPACE;

class Client
{
public:
    Client(const std::shared_ptr<grpc::Channel> channel, std::string_view id)
        : mStub(ClapInterface::NewStub(channel)), mId(id), tCreate(Timestamp::stamp())
    {}

    void serverEventStreamHandler()
    {
        ClientRequest request;
        ServerEvents serverEvents;
        grpc::ClientContext context;
        context.AddMetadata(Metadata::PluginHashId.data(), mId);
        auto stream = mStub->ServerEventStream(&context, request);

        std::vector<Stamp> stamps;
        stamps.reserve(3);
        uint64_t bytesWritten = 0;
        uint64_t messageCount = 0;

        Stamp tBegin = Timestamp::stamp();
        while (stream->Read(&serverEvents)) {
            bytesWritten += serverEvents.ByteSizeLong();
            messageCount += static_cast<uint64_t>(serverEvents.events_size());
            if (serverEvents.has_timestamp()) {
                stamps.emplace_back(serverEvents.timestamp().seconds(), serverEvents.timestamp().nanos());
            }
        }
        Stamp tEnd = Timestamp::stamp();

        using std::cout;
        using std::endl;

        cout << "####### Client stream finished #########" << endl;
        cout << "Number of messages: " << messageCount << endl;
        cout << "Bytes written: " << bytesWritten << endl;
        cout << "Avg. Bytes/Messages: " << static_cast<double>(bytesWritten/messageCount) << endl;
        cout << endl;
        cout << "Stamps: " << stamps.size() << endl;
        if (stamps.size() == 2) {
            stamps[1].subtractMilliseconds(1); // The 1ms we wait in the host.
            const auto serverRtt = tEnd.delta(stamps[0]);
            cout << "Server RTT: " << serverRtt.toDouble() << " s" << endl;
            cout << "Mbps: " << (static_cast<double>(bytesWritten * 8)) / serverRtt.toDouble() / 1e6 << endl;
            auto serverRttMicros = serverRtt.toDouble() * 1e6;
            cout << "Time/Message: " << serverRttMicros / static_cast<double>(messageCount) << "us" << endl;
        }

        if (auto s = stream->Finish(); !s.ok())
            std::cout << "Channel error: " << s.error_message() << std::endl;
    }

private:
    std::unique_ptr<ClapInterface::Stub> mStub;
    std::string mId;
    Stamp tCreate;
};

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "Usage: client-cpp <plugin-id>" << std::endl;
        return 1;
    }

    for (int i = 0; i < argc; ++i)
        std::cout << argv[i] << std::endl;

    Client client(grpc::CreateChannel(argv[2], grpc::InsecureChannelCredentials()), argv[1]);
    client.serverEventStreamHandler();
    return 0;
}
