#pragma once

#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Shared/datatypes.h" // Para as structs de dados
#include "Logging/LogEvents.h"   // Para os enums de log

// Forward declarations para as dependências que serão injetadas
class DatabaseManager;
class LogService;
class CharacterService;
class UserService;
class ClientSession;

// Uma struct para representar uma requisição de login na fila
struct LoginRequest {
    std::shared_ptr<ClientSession> session;
    std::string accountName;
    std::string password;
    std::string ipAddress;
    std::string macAddress;
    std::string pcName;
    std::string serialHD;
    std::string hardwareID;
    std::string videoName;
    std::string widthScreen;
    std::string heightScreen;
    std::string systemOS;

};

class AccountService {
public:
    // Construtor com Injeção de Dependência
    AccountService(DatabaseManager& dbManager, LogService& logService, CharacterService& charService, UserService& userService);
    ~AccountService();

    // Desabilita cópia para evitar problemas com a thread
    AccountService(const AccountService&) = delete;
    AccountService& operator=(const AccountService&) = delete;

    /**
     * @brief Adiciona uma nova requisição de login à fila para processamento assíncrono.
     * @param request A requisição de login a ser processada.
     */
    void queueLoginRequest(LoginRequest request);

    /**
     * @brief Manipula a seleção de um personagem por um usuário já autenticado.
     * @param session A sessão do cliente.
     * @param charName O nome do personagem selecionado.
     */
    void handleSelectCharacter(std::shared_ptr<ClientSession> session, const std::string& charName);

    // ... outros métodos públicos que o AccountService precisa expor ...

private:
    /**
     * @brief O loop principal da thread de trabalho que processa a fila de logins.
     */
    void processLoginQueue();

    /**
     * @brief A lógica principal que processa uma única requisição de login.
     * @param request A requisição a ser processada.
     */
    void processSingleLogin(LoginRequest& request);

    // Funções auxiliares para quebrar a lógica complexa de login
    Log::LoginResult checkAccountStatus(const LoginRequest& request);
    bool validatePassword(const std::string& plainPassword, const std::string& hashedPassword);
    // ... outras funções de verificação (bans, manutenção, etc.)

    // --- Dependências Injetadas ---
    DatabaseManager& m_dbManager;
    LogService& m_logService;
    CharacterService& m_characterService;
    UserService& m_userService;

    // --- Lógica da Fila de Login ---
    std::queue<LoginRequest> m_loginQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::thread m_workerThread;
    bool m_isStopping = false;
};