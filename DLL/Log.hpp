#pragma once

#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <format>
#include <mutex>
#include <share.h>

/// <summary>
/// Classification of the logs level.
/// </summary>
enum class LogLevel : unsigned char {
    Debug,
    Info,
    Warning,
    Error
};

namespace LogSettings {
    /// <summary>
    /// The time that we have that we consider "boot time" of Rocksmith.
    /// All timestamps in the log are based off of this value.
    /// </summary>
    inline clock_t startupTime;

    /// <summary>
    /// Log level to default to, in-case we need to add more values in the future.
    /// </summary>
    inline LogLevel defaultLogLevel = LogLevel::Info;

    inline constexpr bool isDebugBuild = 
    #if defined(_DEBUG) || defined(_WWISE_LOGS)
        true;
    #else
        false;
    #endif
}

/// <summary>
/// Singleton logger that manages both file and console output.
/// </summary>
class Logger {
public:
    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    ~Logger() {
        std::lock_guard lock(mutex);
        if (fileHandle) {
            fclose(fileHandle);
            fileHandle = nullptr;
        }
    }

    /// <summary>
    /// Open a log file on disk with full sharing permissions (_SH_DENYNO) so
    /// external applications and users can view it while the game is running.
    /// </summary>
    void InitFile(const std::string& path) {
        std::lock_guard lock(mutex);
        if (fileHandle) {
            fclose(fileHandle);
            fileHandle = nullptr;
        }
        fileHandle = _fsopen(path.c_str(), "w", _SH_DENYNO);
    }

    /// <summary>
    /// Log a message with automatic header generation.
    /// </summary>
    template <typename T>
    void Log(const T& message, LogLevel level = LogSettings::defaultLogLevel) {
        std::lock_guard lock(mutex);

        std::string header = GenerateHeader(level);

        if (fileHandle) {
            std::ostringstream ss;
            ss << header << message;
            const std::string str = ss.str();
            fwrite(str.data(), 1, str.size(), fileHandle);
            fflush(fileHandle);
        }

        std::cerr << header << message;

        if constexpr (LogSettings::isDebugBuild) {
            std::cout << message;
        }
    }

    /// <summary>
    /// Log without header (for continuation of previous message).
    /// </summary>
    template <typename T>
    void LogNoHeader(const T& message) {
        std::lock_guard lock(mutex);

        if (fileHandle) {
            std::ostringstream ss;
            ss << message;
            const std::string str = ss.str();
            fwrite(str.data(), 1, str.size(), fileHandle);
            fflush(fileHandle);
        }

        std::cerr << message;

        if constexpr (LogSettings::isDebugBuild) {
            std::cout << message;
        }
    }

    /// <summary>
    /// Set the current log level for subsequent logs.
    /// </summary>
    void SetLevel(LogLevel level) {
        currentLevel = level;
    }

    LogLevel GetLevel() const {
        return currentLevel;
    }

private:
    Logger() = default;

    std::string GenerateHeader(LogLevel level) {
        double timeSinceStart = static_cast<double>(clock() - LogSettings::startupTime) / CLOCKS_PER_SEC;

        const char* levelStr;
        switch (level) {
            case LogLevel::Debug:   levelStr = "[DEBUG] ";   break;
            case LogLevel::Info:    levelStr = "[INFO] ";    break;
            case LogLevel::Warning: levelStr = "[WARNING] "; break;
            case LogLevel::Error:   levelStr = "[ERROR] ";   break;
            default:                levelStr = "[UNKNOWN] "; break;
        }

        return std::format("{:.2f} {}", timeSinceStart, levelStr);
    }

    LogLevel currentLevel = LogSettings::defaultLogLevel;
    FILE* fileHandle = nullptr;
    std::mutex mutex; 
};

#define LOG_DEBUG(msg)   do { std::ostringstream _log_ss; _log_ss << msg; Logger::GetInstance().Log(_log_ss.str(), LogLevel::Debug); } while(0)
#define LOG_INFO(msg)    do { std::ostringstream _log_ss; _log_ss << msg; Logger::GetInstance().Log(_log_ss.str(), LogLevel::Info); } while(0)
#define LOG_WARNING(msg) do { std::ostringstream _log_ss; _log_ss << msg; Logger::GetInstance().Log(_log_ss.str(), LogLevel::Warning); } while(0)
#define LOG_ERROR(msg)   do { std::ostringstream _log_ss; _log_ss << msg; Logger::GetInstance().Log(_log_ss.str(), LogLevel::Error); } while(0)
#define LOG_NOHEAD(msg)  do { std::ostringstream _log_ss; _log_ss << msg; Logger::GetInstance().LogNoHeader(_log_ss.str()); } while(0)

#define LOG_SETLEVEL(lvl) Logger::GetInstance().SetLevel(lvl)