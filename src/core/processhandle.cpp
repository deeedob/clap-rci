#include "processhandle.h"

#include <utility>

RCLAP_BEGIN_NAMESPACE

struct ProcessHandlePrivate
{
    ProcessHandlePrivate() = default;
    explicit ProcessHandlePrivate(std::filesystem::path  path, const std::vector<std::string>& args = {});

    bool isValid() const;
    bool isChildRunning() const;
    std::optional<int> checkChildStatus();
    std::optional<int> waitForChild();
    bool startChild();
    std::optional<int> terminateChild();
    std::optional<int> unwrapStatus(int rawStatus);

    std::filesystem::path mPath;
    std::vector<std::string> mArgs;

#if defined _WIN32 || defined _WIN64
    HANDLE mChildHandle = nullptr;
#elif defined __linux__ || defined __APPLE__
    pid_t mChildHandle = -1;
    // ... other Unix-specific members ...
#endif
};

ProcessHandle::ProcessHandle() : dPtr(std::make_unique<ProcessHandlePrivate>()) {}

ProcessHandle::ProcessHandle(const std::filesystem::path& path, const std::vector<std::string>& args)
    : dPtr(std::make_unique<ProcessHandlePrivate>(path, args)) {}
    
ProcessHandle::~ProcessHandle()
{
    if (dPtr->isChildRunning())
        terminateChild();
}

ProcessHandle::ProcessHandle(ProcessHandle &&other) noexcept
    : dPtr(std::move(other.dPtr)) {}

ProcessHandle& ProcessHandle::operator=(ProcessHandle &&other) noexcept
{
    if (this != &other)
        dPtr = std::move(other.dPtr);
    return *this;
}

ProcessHandle::operator bool() const
{
    return isValid();
}

bool ProcessHandle::isValid() const
{
    return dPtr->isValid();
}

bool ProcessHandle::isChildRunning() const
{
    return dPtr->isChildRunning();
}

std::optional<int> ProcessHandle::checkChildStatus()
{
    return dPtr->checkChildStatus();
}

std::optional<int> ProcessHandle::waitForChild()
{
    return dPtr->waitForChild();
}

const std::filesystem::path& ProcessHandle::getPath() const
{
    return dPtr->mPath;
}

const std::vector<std::string>& ProcessHandle::getArguments() const
{
    return dPtr->mArgs;
}

bool ProcessHandle::setArguments(const std::vector<std::string> &args)
{
    if (dPtr->isChildRunning())
        return false;
    dPtr->mArgs = args;
    return true;
}

bool ProcessHandle::setExecutable(const std::filesystem::path &path)
{
    if (dPtr->isChildRunning() || !std::filesystem::exists(path))
        return false;
    dPtr->mPath = path;
    return true;
}

void ProcessHandle::clearArguments()
{
    dPtr->mArgs.clear();
}

PidType ProcessHandle::getCurrentPid()
{
#if defined _WIN32 || defined _WIN64
    return GetCurrentProcessId();
#elif defined __linux__ || defined __APPLE__
    return getpid();
#endif
}

PidType ProcessHandle::getParentPid()
{
#if defined _WIN32 || defined _WIN64
    // Windows doesn't have a direct API to get the parent process ID.
    // For simplicity, we'll return the current process ID as a placeholder.
    return GetCurrentProcessId();
#elif defined __linux__ || defined __APPLE__
    return getppid();
#endif
}

PidType ProcessHandle::getChildPid() const
{
#if defined _WIN32 || defined _WIN64
    return GetProcessId(dPtr->mChildHandle);
#elif defined __linux__ || defined __APPLE__
    return dPtr->mChildHandle;
#endif
}

bool ProcessHandle::startChild()
{
    return dPtr->startChild();
}

std::optional<int> ProcessHandle::terminateChild()
{
    return dPtr->terminateChild();
}

//
// ############### PRIVATE IMPLEMENTATION ###############
//
ProcessHandlePrivate::ProcessHandlePrivate(std::filesystem::path path, const std::vector<std::string>& args)
    : mPath(std::move(path)), mArgs(args) {}

bool ProcessHandlePrivate::isValid() const
{
    return !mPath.empty() && std::filesystem::exists(mPath) && !isChildRunning();
}

bool ProcessHandlePrivate::isChildRunning() const
{
#if defined _WIN32 || defined _WIN64
    if (mChildHandle == nullptr)
        return false;
    DWORD exitCode;
    if (!GetExitCodeProcess(mChildHandle, &exitCode))
        return false;
    return exitCode == STILL_ACTIVE;

#elif defined __linux__ || defined __APPLE__
    return mChildHandle != -1;
#endif
}

std::optional<int> ProcessHandlePrivate::checkChildStatus()
{
#if defined _WIN32 || defined _WIN64
    // Windows doesn't have a direct non-blocking check like waitpid with WNOHANG.
    // Instead, we can use WaitForSingleObject with a timeout of 0.
    if (mChildHandle == nullptr)
        return std::nullopt;
    DWORD result = WaitForSingleObject(mChildHandle, 0);
    if (result == WAIT_OBJECT_0) {
        DWORD exitCode;
        if (!GetExitCodeProcess(mChildHandle, &exitCode))
            return std::nullopt;
        CloseHandle(mChildHandle);
        mChildHandle = nullptr;
        return static_cast<int>(exitCode);
    }
    return std::nullopt;

#elif defined __linux__ || defined __APPLE__

    if (mChildHandle == -1)
        return std::nullopt;
    int status;
    pid_t result = waitpid(mChildHandle, &status, WNOHANG);
    if (result == mChildHandle)
    {
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        if (WIFSIGNALED(status))
            return -WTERMSIG(status);
    }
    return std::nullopt;
#endif
}

std::optional<int> ProcessHandlePrivate::waitForChild()
{
#if defined _WIN32 || defined _WIN64
    if (mChildHandle == nullptr)
        return std::nullopt;
    WaitForSingleObject(mChildHandle, INFINITE);
    DWORD exitCode;
    if (!GetExitCodeProcess(mChildHandle, &exitCode))
        return std::nullopt;
    CloseHandle(mChildHandle);
    mChildHandle = nullptr;

    return static_cast<int>(exitCode);

#elif defined __linux__ || defined __APPLE__

    if (mChildHandle == -1)
        return std::nullopt;
    int status;
    waitpid(mChildHandle, &status, 0);
    const auto unwrappedStatus = unwrapStatus(status);
    if (!unwrappedStatus)
        return std::nullopt;

    mChildHandle = -1;
    return *unwrappedStatus;
#endif
}

bool ProcessHandlePrivate::startChild()
{
#if defined _WIN32 || defined _WIN64
    STARTUPINFOA si = {
        .cb = sizeof(STARTUPINFOA),
        .lpReserved = 0, .lpDesktop = 0, .lpTitle = 0,
        .dwX = (DWORD)CW_USEDEFAULT,.dwY = (DWORD)CW_USEDEFAULT,
        .dwXSize = (DWORD)CW_USEDEFAULT,.dwYSize = (DWORD)CW_USEDEFAULT,
        .dwXCountChars = 0,.dwYCountChars = 0,.dwFillAttribute = 0,
        .dwFlags = STARTF_USESTDHANDLES,
        .wShowWindow = 0,.cbReserved2 = 0,
        .hStdInput = GetStdHandle(STD_INPUT_HANDLE),
        .hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE),
        .hStdError = GetStdHandle(STD_ERROR_HANDLE),
    };
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    // Resolve symlink
    std::filesystem::path resolvedPath = mPath;
    if (std::filesystem::is_symlink(mPath)) {
        resolvedPath = std::filesystem::read_symlink(mPath);
    }
    std::string workingDir = resolvedPath.parent_path().string();

    std::string appPathStr = resolvedPath.string();
    std::string argsStr;
    for (const auto& arg : mArgs) {
        argsStr += "\"" + arg + "\" ";
    }
    if (!argsStr.empty()) {
        argsStr.pop_back();  // Remove the trailing space
    }
    std::string cmdLine = "\"" + appPathStr + "\" " + argsStr;
    LPSTR lpApplicationName = const_cast<char*>(appPathStr.c_str());
    LPSTR lpCommandLine = const_cast<char*>(cmdLine.c_str());
    SPDLOG_INFO("Starting app: {}, cmd: {}", appPathStr, cmdLine);

    // Inherit the environment of the parent process by passing nullptr for lpEnvironment
    if (!CreateProcessA(
        nullptr,
        lpCommandLine,
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        workingDir.c_str(),
        &si,
        &pi)
        ) {
        DWORD error = GetLastError();
        LPVOID errorMsg;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr,
            error,
            0, // Default language
            (LPSTR)&errorMsg,
            0,
            nullptr
        );
        SPDLOG_WARN("Failed to start child process: {}", (char*)errorMsg);
        LocalFree(errorMsg);
        return false;
    }
    CloseHandle(pi.hThread);
    mChildHandle = pi.hProcess;
    return true;
#elif defined __linux__ || defined __APPLE__
    if (!isValid())
        return false;
    mChildHandle = fork();
    if (mChildHandle < 0)
        return false;
    if (mChildHandle == 0) { // Child process
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(mPath.c_str()));
        for (const auto &arg : mArgs)
            argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);
        execv(mPath.c_str(), argv.data());
        SPDLOG_ERROR("Child process failed to start");
        exit(1); // Exit the child's process
    }
    return true;
#endif
}

std::optional<int> ProcessHandlePrivate::terminateChild()
{
#if defined _WIN32 || defined _WIN64
    if (mChildHandle == nullptr)
        return std::nullopt;

    if (!TerminateProcess(mChildHandle, 0))
        return std::nullopt;

    // Wait for the process to finish
    WaitForSingleObject(mChildHandle, INFINITE);

    DWORD exitCode;
    if (!GetExitCodeProcess(mChildHandle, &exitCode)) {
        CloseHandle(mChildHandle);
        mChildHandle = nullptr;
        return std::nullopt;
    }

    CloseHandle(mChildHandle);
    mChildHandle = nullptr;
    return static_cast<int>(exitCode);

#elif defined __linux__ || defined __APPLE__

    if (mChildHandle == -1)
        return std::nullopt;

    if (kill(mChildHandle, SIGTERM) != 0)
        return std::nullopt;

    // Wait for the process to finish and get the exit code
    int status;
    if (waitpid(mChildHandle, &status, 0) == -1) {
        SPDLOG_ERROR("Failed to wait for child: {}", strerror(errno));
        mChildHandle = -1;
        return std::nullopt;
    }

    const auto unwrappedStatus = unwrapStatus(status);
    if (!unwrappedStatus)
        return std::nullopt;

    mChildHandle = -1;
    return *unwrappedStatus;
#endif
}

std::optional<int> ProcessHandlePrivate::unwrapStatus(int rawStatus)
{
    // unwrap unix status
    if (WIFEXITED(rawStatus))
        rawStatus = WEXITSTATUS(rawStatus);
    else if (WIFSIGNALED(rawStatus))
        rawStatus = WTERMSIG(rawStatus);
    else
        return std::nullopt;
    return rawStatus;
}


RCLAP_END_NAMESPACE