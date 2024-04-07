#include <clap-rci/coreplugin.h>
#include <clap-rci/descriptor.h>
#include <clap-rci/registry.h>
#include <clap-rci/server.h>

#include <absl/log/log.h>

#include <csignal>
#include <thread>
#include <iostream>

using namespace CLAP_RCI_NAMESPACE;
using namespace std::chrono_literals;

volatile std::sig_atomic_t status = 0;

void signalHandler(int sig)
{
    LOG(INFO) << std::format("signal {} received", sig);
    status = sig;
}

class TestPlugin : public CorePlugin {
    REGISTER;
public:
    explicit TestPlugin(const clap_host* host)
        : CorePlugin(descriptor(), host)
    {
    }
    static const Descriptor *descriptor() noexcept
    {

        static Descriptor desc(
            "clap.rci.one", "TestPluginOne", "vendorone", "0.0.1"
        );
        return &desc;
    }

private:
};

int main()
{
    (void)std::signal(SIGINT, signalHandler);
    clap_host host = {};
    auto temp = std::make_unique<TestPlugin>(&host);
    auto pptr = temp.get();
    PluginInstances::emplace(
        TestPlugin::descriptor()->id(), std::move(temp)
    );

    int num = 0;
    while (!status) {
        std::cout << "\nType a command num: ";
        std::cin >> num;
        switch (num) {
            case 1:
                pptr->destroy();
                break;
            case 2:
                pptr->activate();
                break;
            case 3:
                pptr->deactivate();
                break;
            case 4:
                pptr->startProcessing();
                break;
            case 5:
                pptr->stopProcessing();
                break;
            case 6:
                pptr->reset();
                break;
            default:
                LOG(ERROR) << "Invalid num: " << num;
                break;
        }
        std::cout << " ";
    }

    LOG(INFO) << "Finished";

    return 0;
}
