#pragma once

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <optional>

#include "Shared/datatypes.h" // Para structs como PacketLoginUser e SQLUser
#include "Logging/LogEvents.h"   // Para os enums de log

// Forward declarations para as dependências que serão injetadas
class DatabasePool;
class CharacterService;
class UserService;
class LogService;
class ClientSession;
class ServerConfig;

// Uma struct para representar uma requisição de login na fila de processamento
struct LoginRequest {
    std::shared_ptr<ClientSession> session;
    PacketLoginUser packet;
    std::string ipAddress;
};

class AccountService {
public:
    explicit AccountService(DatabasePool& dbPool, CharacterService& charService, UserService& userService, LogService& logService, const ServerConfig& serverConfig);

    ~AccountService();

    AccountService(const AccountService&) = delete;
    AccountService& operator=(const AccountService&) = delete;

    void queueLoginRequest(std::shared_ptr<ClientSession> session, const PacketLoginUser& packet);

private:
    void workerLoop();
    void processLogin(LoginRequest& request);

    // --- Database Helpers ---
    std::optional<SQLUser> getSqlUserInfo(const std::string& accountName);

    // --- Validation Helpers ---
    Log::LoginResult validateRequest(const LoginRequest& request, const SQLUser& sqlUser);

    void sendServerList(std::shared_ptr<ClientSession> session, int ticket);

    // --- Action Helpers ---
    void onLoginSuccess(std::shared_ptr<ClientSession> session, const SQLUser& sqlUser);
    void onLoginFailure(std::shared_ptr<ClientSession> session, Log::LoginResult reason, const std::string& accountName);

    // --- Packet Senders ---
    void sendLoginResult(std::shared_ptr<ClientSession> session, Log::LoginResult code, const std::string& message = "");
    void sendCharacterList(std::shared_ptr<ClientSession> session, const std::string& accountName);

    // --- Dependencies & State ---
    DatabasePool& m_dbPool;
    CharacterService& m_characterService;
    UserService& m_userService;
    LogService& m_logService;
    const ServerConfig& m_serverConfig;

    std::queue<LoginRequest> m_loginQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::thread m_workerThread;
    std::atomic<bool> m_isStopping{ false };
};