#ifndef PATHPROVIDER_H
#define PATHPROVIDER_H

#include <core/global.h>

#include <string_view>
#include <filesystem>
#include <optional>

RCLAP_BEGIN_NAMESPACE

class Settings
{
public:
    Settings() = default;
    explicit Settings(std::string_view pluginPath) noexcept(false)
        : mClapPath(pluginPath)
    {
        std::filesystem::path p(pluginPath);
        if (p.extension() != ".clap")
            throw(std::invalid_argument("Plugin path must have the extension .clap!"));

        // Take the pluginPath without extension as the default plugin directory.
        withPluginDir(p.parent_path().string());
        withLogDir((std::filesystem::path(mPluginDirectory) / "logs").string(), true);
    }

    static std::string clapDir() noexcept
    {
#if defined _WIN32 || defined _WIN64
        static const std::string ClapDir = []() -> std::string {
            char* commonProgramFiles = nullptr;
            size_t len = 0;
            errno_t err = _dupenv_s(&commonProgramFiles, &len, "COMMONPROGRAMFILES");
            if (err == 0 && commonProgramFiles) {
                std::string path = std::string(commonProgramFiles) + R"(\CLAP)";
                free(commonProgramFiles);
                return path;
            }
            return "";
        }();
#elif defined __linux__
        static constexpr std::string_view ClapDir = "/usr/lib/.clap";
#elif defined() __APPLE__
        static constexpr std::string_view ClapDir = "/Library/Audio/Plug-Ins/CLAP";
#endif
        return ClapDir.data();
    }

    static std::string clapUserDir() noexcept
    {
#if defined _WIN32 || defined _WIN64
    static const std::string ClapUserDir = []() -> std::string {
        char* commonProgramFiles = nullptr;
        size_t len = 0;
        errno_t err = _dupenv_s(&commonProgramFiles, &len, "LOCALAPPDATA");
        if (err == 0 && commonProgramFiles) {
            std::string path = std::string(commonProgramFiles) + R"(\Programs\Common\CLAP)";
            free(commonProgramFiles);
            return path;
        }
        return "";
    }();
#elif defined __linux__
    static constexpr std::string_view ClapUserDir = "~/.clap";
#elif defined() __APPLE__
    static constexpr std::string_view ClapUserDir = "/Library/Audio/Plug-Ins/CLAP";
#endif
        return ClapUserDir.data();
    }

    [[nodiscard]] bool hasExecutable() const noexcept
    {
        return std::filesystem::exists(mGuiExePath);
    }

    Settings &withExecutablePath(std::string_view binaryPath) noexcept(false)
    {
        if (!std::filesystem::exists(binaryPath))
            throw(std::runtime_error("Could not find binary"));
        mGuiExePath = binaryPath;
        return *this;
    }

    Settings &withPluginDirExecutablePath(std::string_view binaryPath) noexcept(false)
    {
        const auto pd = pluginDirectory();
        if (!pd)
            throw(std::runtime_error("Could not find plugin directory"));
        auto p = std::filesystem::path(*pd) / binaryPath;
#if defined _WIN32 || defined _WIN64
        if (p.extension() != ".exe")
            p += ".exe";
#endif
        if (!std::filesystem::exists(p))
            throw(std::runtime_error("Could not find binary"));
        mGuiExePath = p.string();
        return *this;
    }

    Settings &withLogDir(std::string_view logPath, bool create = true) noexcept(false)
    {
        if (create && !createDir(logPath))
            throw(std::runtime_error("Could not create directory for plugin logs!"));
        else if (!create && !std::filesystem::exists(logPath))
            throw(std::runtime_error("Could not find log file!"));
        mLogDir = logPath;
        return *this;
    }

    Settings &withPluginDir(std::string_view pluginDataDir, bool create = true) noexcept(false)
    {
        if (create && !createDir(pluginDataDir))
            throw(std::runtime_error("Could not create directory for plugin data!"));
        else if (!create && !std::filesystem::exists(pluginDataDir))
            throw(std::runtime_error("Could not find plugin data directory!"));
        mPluginDirectory = pluginDataDir;
        return *this;
    }

    std::string_view clapPath() const
    {
        return mClapPath;
    }

    std::optional<std::string_view> executable() const
    {
        if (!std::filesystem::exists(mGuiExePath))
            return std::nullopt;
        return mGuiExePath;
    }

    std::optional<std::string_view> logDirectory() const
    {
        if (!std::filesystem::exists(mLogDir))
            return std::nullopt;
        return mLogDir;
    }

    std::optional<std::string> logFile()
    {
        if (!std::filesystem::exists(mLogDir))
            return std::nullopt;
        return (std::filesystem::path(mLogDir) / mlogFile).string();
    }

    std::optional<std::string_view> pluginDirectory() const
    {
        if (!std::filesystem::exists(mPluginDirectory))
            return std::nullopt;
        return mPluginDirectory;
    }

private:
    [[nodiscard]] static bool createDir(std::string_view path)
    {
        if (!std::filesystem::exists(path)) {
            if (!std::filesystem::create_directory(path))
                return false;
        }
        return true;
    }

private:
    std::string mClapPath;
    std::string mGuiExePath;

    std::string mPluginDirectory;
    std::string mLogDir;
    std::string mlogFile = "plugin.log";
};

RCLAP_END_NAMESPACE

#endif // PATHPROVIDER_H
