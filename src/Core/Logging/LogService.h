#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <format>
#include <utility>

// Forward declaration
struct GlobalState;

enum class LogLevel {
    Info,
    Status,
    Warn,
    Error,
    Debug,
    Packet
};

/**
 * @class LogService
 * @brief Um serviço de logging moderno, thread-safe e type-safe.
 * Gerencia a escrita de logs para um arquivo e para o console (sem cores).
 */
class LogService {
public:
    explicit LogService(const std::string& filename, GlobalState& globalState);
    ~LogService();

    LogService(const LogService&) = delete;
    LogService& operator=(const LogService&) = delete;

    // Métodos de Logging Públicos (sem alterações na interface)
    template<typename... Args>
    void info(const std::string& format_str, Args&&... args) {
        log(LogLevel::Info, std::vformat(format_str, std::make_format_args(args...)));
    }

    template<typename... Args>
    void status(const std::string& format_str, Args&&... args) {
        log(LogLevel::Status, std::vformat(format_str, std::make_format_args(args...)));
    }

    template<typename... Args>
    void warn(const std::string& format_str, Args&&... args) {
        log(LogLevel::Warn, std::vformat(format_str, std::make_format_args(args...)));
    }

    template<typename... Args>
    void error(const std::string& format_str, Args&&... args) {
        log(LogLevel::Error, std::vformat(format_str, std::make_format_args(args...)));
    }

    template<typename... Args>
    void debug(const std::string& format_str, Args&&... args) {
#ifdef DEV_MODE
        log(LogLevel::Debug, std::vformat(format_str, std::make_format_args(args...)));
#endif
    }

    template<typename... Args>
    void packet(const std::string& format_str, Args&&... args) {
#ifdef DEV_MODE
        log(LogLevel::Packet, std::vformat(format_str, std::make_format_args(args...)));
#endif
    }

private:
    void log(LogLevel level, const std::string& message);

    GlobalState& m_globalState;
    std::ofstream m_logFile;
    std::mutex m_mutex;
    // O HANDLE do console foi removido
};