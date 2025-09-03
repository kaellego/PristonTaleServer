#include "Logging/LogService.h"
#include "Shared/GlobalState.h"
#include <iostream>
#include <chrono>
#include <iomanip>

// Função auxiliar para obter a data e hora formatadas
static std::string getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
    localtime_s(&buf, &in_time_t);
    std::stringstream ss;
    ss << std::put_time(&buf, "%Y-%m-%d %X");
    return ss.str();
}

LogService::LogService(const std::string& filename, GlobalState& globalState)
    : m_globalState(globalState), m_logFile(filename, std::ios::out | std::ios::app)
{
    // Não precisamos mais do GetStdHandle
    if (!m_logFile.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o arquivo de log: " + filename);
    }
}

LogService::~LogService() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

// O método central, agora sem a lógica de cores
void LogService::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string prefix;
    switch (level) {
    case LogLevel::Info:    prefix = "[INFO]   "; break;
    case LogLevel::Status:  prefix = "[STATUS] "; break;
    case LogLevel::Warn:    prefix = "[WARN]   "; break;
    case LogLevel::Error:   prefix = "[ERROR]  "; break;
    case LogLevel::Debug:   prefix = "[DEBUG]  "; break;
    case LogLevel::Packet:  prefix = "[PACKET] "; break;
    }

    // Contadores de erro/aviso podem ser mantidos se você os adicionar ao GlobalState

    std::string timestamp = getCurrentDateTime();
    std::string fullLogMessage = timestamp + " " + prefix + message;

    // Escreve no arquivo de log
    m_logFile << fullLogMessage << std::endl;

    // Escreve no console (sem cores)
    std::cout << prefix << message << std::endl;
}