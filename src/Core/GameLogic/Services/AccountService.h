#pragma once

#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Shared/datatypes.h" // Para as structs de dados
#include "Logging/LogEvents.h"   // Para os enums de log

// Forward declarations para as depend�ncias que ser�o injetadas
class DatabaseManager;
class LogService;
class CharacterService;
class UserService;
class ClientSession;

// Uma struct para representar uma requisi��o de login na fila
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
    // Construtor com Inje��o de Depend�ncia
    AccountService(DatabaseManager& dbManager, LogService& logService, CharacterService& charService, UserService& userService);
    ~AccountService();

    // Desabilita c�pia para evitar problemas com a thread
    AccountService(const AccountService&) = delete;
    AccountService& operator=(const AccountService&) = delete;

    /**
     * @brief Adiciona uma nova requisi��o de login � fila para processamento ass�ncrono.
     * @param request A requisi��o de login a ser processada.
     */
    void queueLoginRequest(LoginRequest request);

    /**
     * @brief Manipula a sele��o de um personagem por um usu�rio j� autenticado.
     * @param session A sess�o do cliente.
     * @param charName O nome do personagem selecionado.
     */
    void handleSelectCharacter(std::shared_ptr<ClientSession> session, const std::string& charName);

    // ... outros m�todos p�blicos que o AccountService precisa expor ...

private:
    /**
     * @brief O loop principal da thread de trabalho que processa a fila de logins.
     */
    void processLoginQueue();

    /**
     * @brief A l�gica principal que processa uma �nica requisi��o de login.
     * @param request A requisi��o a ser processada.
     */
    void processSingleLogin(LoginRequest& request);

    // Fun��es auxiliares para quebrar a l�gica complexa de login
    Log::LoginResult checkAccountStatus(const LoginRequest& request);
    bool validatePassword(const std::string& plainPassword, const std::string& hashedPassword);
    // ... outras fun��es de verifica��o (bans, manuten��o, etc.)

    // --- Depend�ncias Injetadas ---
    DatabaseManager& m_dbManager;
    LogService& m_logService;
    CharacterService& m_characterService;
    UserService& m_userService;

    // --- L�gica da Fila de Login ---
    std::queue<LoginRequest> m_loginQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::thread m_workerThread;
    bool m_isStopping = false;
};