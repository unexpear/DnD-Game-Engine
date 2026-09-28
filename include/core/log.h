/**
 * @file log.h
 * @brief Thread-safe logging system with console and file output.
 * 
 * Provides a singleton Logger that outputs log messages to:
 *   1. Console stdout with color codes (platform-aware)
 *   2. A log file on disk (appended each session)
 * 
 * Log levels (ascending severity):
 *   TRACE   — Detailed tracing (function entry/exit, loop iterations)
 *   DEBUG   — Debugging info (variable values, state changes)
 *   INFO    — General informational messages (initialization, loading)
 *   WARN    — Warning conditions (deprecated usage, non-critical errors)
 *   ERROR   — Error conditions (failed operations, missing resources)
 *   FATAL   — Critical errors that require immediate shutdown
 * 
 * Thread safety: All logging methods are protected by a mutex.
 * Multiple threads can log simultaneously without data races.
 * 
 * @note TRACE and DEBUG messages can be compiled out in Release builds
 *       by defining DND_LOG_NO_DEBUG at the preprocessor level.
 * @warning The log file is opened in append mode. Previous sessions' logs
 *          are preserved. The file is NOT rotated or truncated automatically.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include <cstdio>
#include <mutex>
#include <cstdarg>

namespace Dnd {

// ==========================================
// ENUM: LogLevel
// ==========================================

/**
 * @enum LogLevel
 * @brief Severity levels for log messages.
 * 
 * Numeric values are sequential (0-5) to enable range-based filtering.
 * Setting the minimum log level to WARN suppresses TRACE, DEBUG, and INFO.
 */
enum class LogLevel : i32 {
    TRACE = 0,   ///< Detailed tracing. Very verbose. Compile-outable in Release.
    DEBUG = 1,   ///< Debugging info. Compile-outable in Release.
    INFO  = 2,   ///< General information. Always logged.
    WARN  = 3,   ///< Warnings. Non-critical issues.
    ERR   = 4,   ///< Errors. Operations that failed but didn't crash.
    FATAL = 5    ///< Fatal errors. Engine will shut down after this.
};

// ==========================================
// CLASS: Logger
// ==========================================

/**
 * @class Logger
 * @brief Singleton logging manager with console and file output.
 * 
 * Implementation details:
 *   - Singleton pattern: one global instance, accessed via static methods
 *   - Mutex-protected: thread-safe logging from any thread
 *   - FILE* based: uses C-style stdio for maximum compatibility
 *   - Console colors: ANSI escape codes on Unix, Win32 API on Windows
 *   - Timestamps: [HH:MM:SS] prefix on every message
 */
class Logger {
public:
    /**
     * @brief Initializes the logging system.
     * 
     * Implementation:
     *   1. Check if already initialized (return true if so)
     *   2. Open the log file in append mode ("a")
     *      - If file open fails, log to console only, return false
     *   3. Initialize console color support on Windows (SetConsoleMode)
     *   4. Write session header to log file
     *   5. Set minimum log level to TRACE (log everything)
     *   6. Set initialized flag = true
     * 
     * @param logFilePath Path to the log file. If nullptr, logs to console only.
     * @return true if initialization succeeded, false if file could not be opened.
     * @note This method is NOT thread-safe. Call once from main thread before any logging.
     */
    static b8 Initialize(const char* logFilePath);

    /**
     * @brief Shuts down the logging system.
     * 
     * Implementation:
     *   1. Check if not initialized (return immediately)
     *   2. Write session footer to log file
     *   3. Close the log file handle with fclose()
     *   4. Set initialized flag = false
     */
    static void Shutdown();

    /**
     * @brief Sets the minimum log level. Messages below this level are discarded.
     * @param level The minimum log level to output.
     */
    static void SetLevel(LogLevel level);

    /**
     * @brief Logs a formatted message at the specified level.
     * 
     * This is the CORE logging function. All macros expand to call this method.
     * 
     * Implementation:
     *   1. Check if message level >= minimum level (skip if below)
     *   2. Lock the mutex for thread safety
     *   3. Get current timestamp as [HH:MM:SS] string
     *   4. Format the message using vsnprintf into a buffer (max 4096 chars)
     *      - If message exceeds buffer, truncate and append "..."
     *   5. Construct full output: "[HH:MM:SS] [LEVEL] message"
     *   6. Output to console with color codes
     *   7. If log file is open, write the same string (without color codes) to file
     *   8. If level is FATAL, flush the file to ensure it's written
     *   9. Unlock mutex
     * 
     * @param level Severity level of the message.
     * @param file Source file name (typically __FILE__).
     * @param line Source line number (typically __LINE__).
     * @param format Printf-style format string.
     * @param ... Format arguments.
     * @warning Buffer size is limited to 4096 characters. Longer messages are truncated.
     */
    static void Log(LogLevel level, const char* file, i32 line, const char* format, ...);

private:
    /// @brief Private constructor. Enforces singleton pattern.
    Logger() = default;

    /// @brief Returns the color code string for a given log level.
    static const char* GetColorCode(LogLevel level);

    /// @brief Returns the string name for a log level.
    static const char* GetLevelName(LogLevel level);

    /// @brief Private log file handle. nullptr if no file logging.
    static FILE* s_fileHandle;
    /// @brief Whether the logger has been initialized.
    static b8 s_initialized;
    /// @brief Minimum log level to output.
    static LogLevel s_minLevel;
    /// @brief Mutex for thread-safe logging.
    static std::mutex s_mutex;
};

// ==========================================
// LOGGING MACROS
// ==========================================

/**
 * @brief Log a TRACE message.
 * @note Compiles to NOTHING if DND_LOG_NO_DEBUG is defined.
 */
#ifdef DND_LOG_NO_DEBUG
    #define DND_LOG_TRACE(...) ((void)0)
#else
    #define DND_LOG_TRACE(...) \
        Dnd::Logger::Log(Dnd::LogLevel::TRACE, __FILE__, __LINE__, __VA_ARGS__)
#endif

/**
 * @brief Log a DEBUG message.
 * @note Compiles to NOTHING if DND_LOG_NO_DEBUG is defined.
 */
#ifdef DND_LOG_NO_DEBUG
    #define DND_LOG_DEBUG(...) ((void)0)
#else
    #define DND_LOG_DEBUG(...) \
        Dnd::Logger::Log(Dnd::LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#endif

/** @brief Log an INFO message. Always active. */
#define DND_LOG_INFO(...) \
    Dnd::Logger::Log(Dnd::LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)

/** @brief Log a WARNING message. Always active. */
#define DND_LOG_WARN(...) \
    Dnd::Logger::Log(Dnd::LogLevel::WARN, __FILE__, __LINE__, __VA_ARGS__)

/** @brief Log an ERROR message. Always active. */
#define DND_LOG_ERROR(...) \
    Dnd::Logger::Log(Dnd::LogLevel::ERR, __FILE__, __LINE__, __VA_ARGS__)

/** @brief Log a FATAL message. Always active. */
#define DND_LOG_FATAL(...) \
    Dnd::Logger::Log(Dnd::LogLevel::FATAL, __FILE__, __LINE__, __VA_ARGS__)

} // namespace Dnd
