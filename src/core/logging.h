#ifndef LOGGING_H
#define LOGGING_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "global.h"

RCLAP_BEGIN_NAMESPACE

namespace Log {
    namespace Private {
        static bool gLoggerSetup = false;
    }
    static constexpr std::string_view LoggerName = "logger";
    static constexpr std::string_view LoggerPattern = "[%=8l][%P♯%t] %v";

    inline std::shared_ptr<spdlog::logger> setupLogger(std::string_view filename)
    {
        if (Log::Private::gLoggerSetup)
            return {};

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        if (!filename.empty())
            sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename.data()));
        auto logger = std::make_shared<spdlog::logger>(Log::LoggerName.data(), begin(sinks), end(sinks));
        // register it if you need to access it globally
        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
        spdlog::set_pattern(Log::LoggerPattern.data());

#ifdef SPDLOG_LEVEL_TRACE
        spdlog::set_level(spdlog::level::trace);
#endif
        Log::Private::gLoggerSetup = true;
        return logger;
    }
}

RCLAP_END_NAMESPACE

#endif // LOGGING_H
