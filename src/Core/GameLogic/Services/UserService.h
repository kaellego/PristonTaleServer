#pragma once

#include <string>
#include <map>
#include <mutex>
#include <memory>
#include "Shared/datatypes.h"

// Forward declarations
class ClientSession;
class LogService;

// Estrutura para manter os dados de um usuário online
struct UserSession {
    int accountId;
    std::string accountName;
    std::shared_ptr<ClientSession> session;
    // Adicione aqui outros dados, como o personagem selecionado, etc.
};

/**
 * @class UserService
 * @brief Gerencia os usuários atualmente online no servidor.
 */
class UserService {
public:
    explicit UserService(LogService& logService);

    /**
     * @brief Adiciona um novo usuário à lista de jogadores online.
     * @param accountId O ID da conta do usuário.
     * @param session A sessão de rede do usuário.
     */
    void addUser(int accountId, std::shared_ptr<ClientSession> session);

    /**
     * @brief Remove um usuário da lista de jogadores online.
     * @param session A sessão de rede do usuário a ser removido.
     */
    void removeUser(std::shared_ptr<ClientSession> session);

    /**
     * @brief Verifica se uma conta já está online.
     * @param accountId O ID da conta a ser verificado.
     * @return true se a conta estiver online, false caso contrário.
     */
    bool isUserOnline(int accountId);

private:
    LogService& m_logService;

    // Um mapa para armazenar as sessões dos usuários online, usando o ID da conta como chave.
    // O std::mutex garante que o acesso ao mapa seja seguro entre múltiplas threads.
    std::map<int, UserSession> m_onlineUsers;
    std::mutex m_usersMutex;
};