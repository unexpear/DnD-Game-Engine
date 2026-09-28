/**
 * @file log.cpp
 * @brief Implementation of the Logger class.
 * 
 * See log.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "core/log.h"
#include <chrono>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
    #undef min
    #undef max
#endif

namespace Dnd {

// ==========================================
// STATIC MEMBER INITIALIZATION
// ==========================================

/** @brief Private log file handle. nullptr if no file logging. */
FILE* Logger::s_fileHandle = nullptr;

/** @brief Whether the logger has been initialized. */
b8 Logger::s_initialized = false;

/** @brief Minimum log level to output. Defaults to TRACE (log everything). */
LogLevel Logger::s_minLevel = LogLevel::TRACE;

/** @brief Mutex for thread-safe logging. */
std::mutex Logger::s_mutex;

// ==========================================
// COLOR CODES
// ==========================================

/**
 * @brief Returns the ANSI color code string for a given log level.
 * 
 * On Windows, these codes are interpreted when virtual terminal
 * processing is enabled via SetConsoleMode. On Unix, they work by default.
 * 
 * Color mapping:
 *   TRACE: gray (dark gray escape)
 *   DEBUG: cyan
 *   INFO:  white (no escape — default terminal color)
 *   WARN:  yellow
 *   ERROR: red
 *   FATAL: bright red + bold
 * 
 * @param level The log level.
 * @return ANSI escape code string. Empty string for INFO (default color).
 */
const char* Logger::GetColorCode(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "\033[90m";   // Bright black (gray)
        case LogLevel::DEBUG: return "\033[36m";   // Cyan
        case LogLevel::INFO:  return "";           // Default (white)
        case LogLevel::WARN:  return "\033[33m";   // Yellow
        case LogLevel::ERR: return "\033[31m";   // Red
        case LogLevel::FATAL: return "\033[1;31m"; // Bold red
        default:              return "";
    }
}

/**
 * @brief Returns the string name for a log level.
 * @param level The log level.
 * @return String: "TRACE", "DEBUG", "INFO", "WARN", "ERROR", or "FATAL".
 */
const char* Logger::GetLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "?????";
    }
}

// ==========================================
// INITIALIZATION
// ==========================================

b8 Logger::Initialize(const char* logFilePath) {
    // Step 1: Check if already initialized
    if (s_initialized) {
        return true;
    }

    // Step 2: Open the log file in append mode
    if (logFilePath != nullptr) {
        s_fileHandle = std::fopen(logFilePath, "a");
        if (s_fileHandle == nullptr) {
            // File could not be opened — log to console only
            std::fprintf(stderr, "[LOGGER] Warning: Could not open log file: %s\n", logFilePath);
            // Continue without file logging — not a fatal error
        }
    }

#ifdef _WIN32
    // Step 3: Enable virtual terminal processing on Windows for ANSI color codes
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hConsole, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hConsole, mode);
        }
    }
#endif

    // Step 4: Write session header to log file
    if (s_fileHandle != nullptr) {
        // Get current time for the header
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::fprintf(s_fileHandle, "=== D&D Engine Log Started [%s] ===\n", std::ctime(&time_t));
        std::fflush(s_fileHandle);
    }

    // Step 5: Minimum level defaults to TRACE (already set at static init)
    // Step 6: Mark as initialized
    s_initialized = true;
    return true;
}

// ==========================================
// SHUTDOWN
// ==========================================

void Logger::Shutdown() {
    // Step 1: Check if not initialized
    if (!s_initialized) {
        return;
    }

    // Step 2: Write session footer to log file
    if (s_fileHandle != nullptr) {
        std::fprintf(s_fileHandle, "=== D&D Engine Log Ended ===\n");
        // Step 3: Close the log file handle
        std::fclose(s_fileHandle);
        s_fileHandle = nullptr;
    }

    // Step 4: Mark as not initialized
    s_initialized = false;
}

// ==========================================
// LOG LEVEL FILTERING
// ==========================================

void Logger::SetLevel(LogLevel level) {
    // Clamp to valid range (shouldn't happen, but defensive)
    if (level < LogLevel::TRACE) level = LogLevel::TRACE;
    if (level > LogLevel::FATAL) level = LogLevel::FATAL;
    s_minLevel = level;
}

// ==========================================
// CORE LOGGING FUNCTION
// ==========================================

void Logger::Log(LogLevel level, const char* file, i32 line, const char* format, ...) {
    // Step 1: Check if message level >= minimum level
    if (level < s_minLevel) {
        return; // Below threshold — discard
    }

    // Step 2: Lock the mutex for thread safety
    std::lock_guard<std::mutex> lock(s_mutex);

    // Step 3: Get current timestamp as [HH:MM:SS]
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;

#ifdef _WIN32
    // Windows: localtime_s is the thread-safe version
    localtime_s(&tm_buf, &time_t);
#else
    // Unix: localtime_r is the thread-safe version
    localtime_r(&time_t, &tm_buf);
#endif

    // Step 4: Format the message using vsnprintf (max 4096 chars)
    constexpr i32 MAX_MSG_LEN = 4096;
    char msgBuffer[MAX_MSG_LEN];

    va_list args;
    va_start(args, format);
    i32 msgLen = std::vsnprintf(msgBuffer, MAX_MSG_LEN, format, args);
    va_end(args);

    // Truncate and append "..." if message exceeds buffer
    if (msgLen >= MAX_MSG_LEN) {
        msgBuffer[MAX_MSG_LEN - 4] = '.';
        msgBuffer[MAX_MSG_LEN - 3] = '.';
        msgBuffer[MAX_MSG_LEN - 2] = '.';
        msgBuffer[MAX_MSG_LEN - 1] = '\0';
    }

    // Step 5: Construct full output string
    const char* resetCode = "\033[0m";
    const char* colorCode = GetColorCode(level);
    const char* levelName = GetLevelName(level);

    // Build the formatted line: [HH:MM:SS] [LEVEL] message (file:line)
    char outputBuffer[MAX_MSG_LEN + 128];
    int outputLen = std::snprintf(outputBuffer, sizeof(outputBuffer),
        "[%02d:%02d:%02d] [%s] %s  (%s:%d)",
        tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
        levelName, msgBuffer, file, line);

    // Step 6: Output to console with color codes
    if (outputLen > 0) {
        // Write color prefix, then the message, then reset
        std::printf("%s%s%s\n", colorCode, outputBuffer, resetCode);
        std::fflush(stdout);
    }

    // Step 7: If log file is open, write the same string WITHOUT color codes
    if (s_fileHandle != nullptr) {
        std::fprintf(s_fileHandle, "%s\n", outputBuffer);

        // Step 8: If FATAL, flush the file to ensure it's written before crash
        if (level == LogLevel::FATAL) {
            std::fflush(s_fileHandle);
        }
    }

    // Step 10: If FATAL, also print to stderr as a fallback
    if (level == LogLevel::FATAL) {
        std::fprintf(stderr, "%s\n", outputBuffer);
    }

    // Mutex unlocks automatically via lock_guard
}

} // namespace Dnd
