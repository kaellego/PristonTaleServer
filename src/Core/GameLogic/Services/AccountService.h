#pragma once

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <optional> // Para std::optional

#include "Shared/datatypes.h" // Para structs como PacketLoginUser e SQLUser
#include "Logging/LogEvents.h"   // Para os enums de log

// Forward declarations para as depend�ncias que ser�o injetadas
class DatabaseManager;
class CharacterService;
class UserService;
class LogService;
class ClientSession;

// Uma struct para representar uma requisi��o de login na fila de processamento
struct LoginRequest {
    std::shared_ptr<ClientSession> session;
    PacketLoginUser packet;
    std::string ipAddress;
};

class AccountService {
public:
    // Construtor com Inje��o de Depend�ncia
    explicit AccountService(DatabaseManager& dbManager, CharacterService& charService, UserService& userService, LogService& logService);
    ~AccountService();

    // Desabilita c�pia/movimenta��o para garantir o gerenciamento correto da thread
    AccountService(const AccountService&) = delete;
    AccountService& operator=(const AccountService&) = delete;

    /**
     * @brief Adiciona uma nova requisi��o de login � fila para ser processada pela thread de trabalho.
     * @param session A sess�o do cliente que est� tentando logar.
     * @param packet O pacote de login recebido do cliente.
     */
    void queueLoginRequest(std::shared_ptr<ClientSession> session, const PacketLoginUser& packet);

    // Adicione outros m�todos p�blicos aqui, como para deletar ou selecionar personagens
    // void handleDeleteCharacter(...)

private:
    /**
     * @brief O loop principal da thread de trabalho que consome a fila de logins.
     */
    void workerLoop();

    /**
     * @brief Processa uma �nica requisi��o de login, realizando todas as valida��es.
     * @param request A requisi��o de login a ser processada.
     */
    void processLogin(LoginRequest& request);

    /**
     * @brief Busca as informa��es de um usu�rio no banco de dados.
     * @param accountName O nome da conta a ser buscada.
     * @return std::optional<SQLUser> contendo os dados do usu�rio, ou vazio se n�o encontrado.
     */
    std::optional<SQLUser> getSqlUserInfo(const std::string& accountName);

    /**
     * @brief Envia um pacote de resposta de login para o cliente.
     * @param session A sess�o do cliente.
     * @param code O c�digo de resultado do login.
     * @param message Uma mensagem opcional (para banimentos tempor�rios).
     */
    void sendLoginResult(std::shared_ptr<ClientSession> session, Log::LoginResult code, const std::string& message = "");

    // --- Depend�ncias Injetadas ---
    DatabaseManager& m_dbManager;
    CharacterService& m_characterService;
    UserService& m_userService;
    LogService& m_logService;

    // --- L�gica da Fila de Login Thread-Safe ---
    std::queue<LoginRequest> m_loginQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::thread m_workerThread;
    std::atomic<bool> m_isStopping{ false };
};