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

// Forward declarations para as dependências que serão injetadas
class DatabaseManager;
class CharacterService;
class UserService;
class LogService;
class ClientSession;

// Uma struct para representar uma requisição de login na fila de processamento
struct LoginRequest {
    std::shared_ptr<ClientSession> session;
    PacketLoginUser packet;
    std::string ipAddress;
};

class AccountService {
public:
    // Construtor com Injeção de Dependência
    explicit AccountService(DatabaseManager& dbManager, CharacterService& charService, UserService& userService, LogService& logService);
    ~AccountService();

    // Desabilita cópia/movimentação para garantir o gerenciamento correto da thread
    AccountService(const AccountService&) = delete;
    AccountService& operator=(const AccountService&) = delete;

    /**
     * @brief Adiciona uma nova requisição de login à fila para ser processada pela thread de trabalho.
     * @param session A sessão do cliente que está tentando logar.
     * @param packet O pacote de login recebido do cliente.
     */
    void queueLoginRequest(std::shared_ptr<ClientSession> session, const PacketLoginUser& packet);

    // Adicione outros métodos públicos aqui, como para deletar ou selecionar personagens
    // void handleDeleteCharacter(...)

private:
    /**
     * @brief O loop principal da thread de trabalho que consome a fila de logins.
     */
    void workerLoop();

    /**
     * @brief Processa uma única requisição de login, realizando todas as validações.
     * @param request A requisição de login a ser processada.
     */
    void processLogin(LoginRequest& request);

    /**
     * @brief Busca as informações de um usuário no banco de dados.
     * @param accountName O nome da conta a ser buscada.
     * @return std::optional<SQLUser> contendo os dados do usuário, ou vazio se não encontrado.
     */
    std::optional<SQLUser> getSqlUserInfo(const std::string& accountName);

    /**
     * @brief Envia um pacote de resposta de login para o cliente.
     * @param session A sessão do cliente.
     * @param code O código de resultado do login.
     * @param message Uma mensagem opcional (para banimentos temporários).
     */
    void sendLoginResult(std::shared_ptr<ClientSession> session, Log::LoginResult code, const std::string& message = "");

    // --- Dependências Injetadas ---
    DatabaseManager& m_dbManager;
    CharacterService& m_characterService;
    UserService& m_userService;
    LogService& m_logService;

    // --- Lógica da Fila de Login Thread-Safe ---
    std::queue<LoginRequest> m_loginQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::thread m_workerThread;
    std::atomic<bool> m_isStopping{ false };
};