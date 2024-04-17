#pragma once

#include <clap-rci/global.h>

#include <memory>
#include <string_view>

CLAP_RCI_BEGIN_NAMESPACE

struct ServerPrivate;

class Server
{
public:
    enum class State { Init = 0, Created, Running, Shutdown, Finished };

    Server();
    ~Server();

    bool start(std::string_view addressUri = "localhost:0");
    bool stop();
    bool reset();

    [[nodiscard]] int port() const noexcept;
    [[nodiscard]] std::string_view address() const noexcept;

private:
    std::unique_ptr<ServerPrivate> dPtr;
};

CLAP_RCI_END_NAMESPACE
