#ifndef PROCESSHANDLER_H
#define PROCESSHANDLER_H

#include "global.h"
#include "logging.h"

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <memory>
#include <optional>

RCLAP_BEGIN_NAMESPACE

#if defined _WIN32 || defined _WIN64
#include <Windows.h>
using PidType = DWORD;
#elif defined __linux__ || defined __APPLE__
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
using PidType = pid_t;
#else
#error "Unsupported platform"
#endif

struct ProcessHandlePrivate;

class ProcessHandle
{
public:
    ProcessHandle();
    explicit ProcessHandle(const std::filesystem::path& path, std::initializer_list<std::string> args = {});
    ~ProcessHandle();

    ProcessHandle(ProcessHandle &&) noexcept;
    ProcessHandle& operator=(ProcessHandle &&) noexcept;

    ProcessHandle(const ProcessHandle &) = delete;
    ProcessHandle& operator=(const ProcessHandle &) = delete;

    operator bool() const;
    bool isValid() const;
    bool isChildRunning() const;
    std::optional<int> checkChildStatus();
    std::optional<int> waitForChild();

    const std::filesystem::path& getPath() const;
    const std::vector<std::string>& getArguments() const;

    bool setArguments(const std::vector<std::string>& args);
    bool setExecutable(const std::filesystem::path& path);
    void clearArguments();

    static PidType getCurrentPid();
    static PidType getParentPid();
    PidType getChildPid() const;

    bool startChild();
    std::optional<int> terminateChild();

private:
    std::unique_ptr<ProcessHandlePrivate> dPtr;
};

RCLAP_END_NAMESPACE

#endif // PROCESSHANDLER_H
