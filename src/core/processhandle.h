#ifndef PROCESSHANDLER_H
#define PROCESSHANDLER_H

#include "global.h"
#include "logging.h"

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

#if defined _WIN32 || defined _WIN64
#error "Windows is not supported at the moment"
#elif defined __linux__ || defined __APPLE__
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
using PidType = pid_t;
#else
#error "Unsupported platform"
#endif

RCLAP_BEGIN_NAMESPACE

class ProcessHandle
{
public:
    ProcessHandle() = default;
    explicit ProcessHandle(std::string path, std::initializer_list<std::string> args = {})
            : mPath(std::move(path)), mArgs(args) { }
    explicit ProcessHandle(PidType pid) : mChildPid(pid) { }
    ~ProcessHandle()
    {
        try {
         if (childRunning())
            killChild();
        } catch (...) {
            SPDLOG_ERROR("Failed to kill child process");
        }
    }

    ProcessHandle(ProcessHandle &&) = default;
    ProcessHandle &operator=(ProcessHandle &&) = default;

    ProcessHandle(const ProcessHandle &) = delete;
    ProcessHandle &operator=(const ProcessHandle &) = delete;

    [[nodiscard]] bool valid() const
    {
        return !mPath.empty() && !childRunning() && canExec();
    }

    [[nodiscard]] bool childRunning() const
    {
        return mChildPid != -1;
    }

    [[nodiscard]] std::optional<int> checkChild()
    {
        if (!childRunning())
            return std::nullopt;
        int status;
        auto pid = waitpid(mChildPid, &status, WNOHANG);
        if (pid <= 0)
            return std::nullopt;

        auto st = unwrapStatus(status);
        if (st)
            resetClient();
        return st;
    }

    std::optional<int> checkChildBlocking()
    {
        if (!childRunning())
            return std::nullopt;

        int status;
        auto pid = waitpid(mChildPid, &status, 0);
        if (pid != mChildPid) {
            SPDLOG_ERROR("Child PID mismatch. Expected: {}, Got: {}", mChildPid, pid);
            return std::nullopt;
        }

        auto st = unwrapStatus(status);
        if (st)
            resetClient();
        return st;
    }

    explicit operator bool() const
    {
        return valid();
    }

    [[nodiscard]] const std::filesystem::path &path() const { return mPath; }
    [[nodiscard]] const std::vector<std::string> &args() const { return mArgs; }

    bool setArgs(std::initializer_list<std::string> args)
    {
        if (childRunning())
            return false;
        mArgs.assign(args);
        return true;
    }

    bool setExecutable(std::string_view path) noexcept
    {
        if (childRunning())
            return false;
        mPath.assign(path);
        if (!canExec()) {
            mPath = "";
            return false;
        }
        return true;
    }

    void clearArgs() noexcept
    {
        mArgs.clear();
    }

    static PidType getPid() { return getpid(); }
    static PidType getParentPid() { return getppid(); }
    [[nodiscard]] PidType getChildPid() const { return mChildPid; }


    [[nodiscard]] bool execute()
    {
        if (!valid())
            return false;

        mChildPid = fork();
        if (mChildPid < 0)
            return false;
        if (mChildPid == 0) { // Child process
            auto args = constructArgsFromThis();
            execv(mPath.c_str(), args.data());
            SPDLOG_ERROR("Child process failed to start");
            exit(1); // Exit the child's process
        } else {
            SPDLOG_DEBUG("Started child on path: {}, PID: {}", path().string(), mChildPid);
            return true;
        }
    }

    bool killChild() const
    {
        return terminate(mChildPid);
    }

    std::optional<int> killChildAndWait()
    {
        if (!killChild())
            return std::nullopt;
        return checkChildBlocking();
    }

    static bool terminate(PidType pid)
    {
        if (pid == -1 || pid == 0)
            return false;
        kill(pid, SIGTERM);
        return true;
    }

private:
    static std::optional<int> unwrapStatus(int rawStatus)
    {
        // unwrap status
        if (WIFEXITED(rawStatus))
            rawStatus = WEXITSTATUS(rawStatus);
        else if (WIFSIGNALED(rawStatus))
            rawStatus = WTERMSIG(rawStatus);
        else
            return std::nullopt;
        return rawStatus;
    }

    void resetClient()
    {
        mChildPid = -1;
    }

    [[nodiscard]] std::vector<char*> constructArgsFromThis() const
    {
        std::vector<char *> argv;
        argv.reserve(mArgs.size() + 2);
        argv.push_back(const_cast<char *>(absolute(mPath).string().data()));
        for (const auto &arg : mArgs)
            argv.push_back(const_cast<char *>(arg.data()));
        argv.push_back(nullptr);
        return argv;
    }

    [[nodiscard]] bool canExec() const
    {
        struct stat st {};

        if (stat(mPath.c_str(), &st) == 0) {
            if ((st.st_mode & S_IXUSR) != 0)
                return true;
        }

        return false;
    }

private:
    PidType mChildPid = -1;

    std::filesystem::path mPath;
    std::vector<std::string> mArgs;
};

RCLAP_END_NAMESPACE

#endif // PROCESSHANDLER_H
