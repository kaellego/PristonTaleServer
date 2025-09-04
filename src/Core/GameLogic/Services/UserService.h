#pragma once

#include <string>
#include <map>
#include <mutex>
#include <memory>
#include "Shared/datatypes.h"

// Forward declarations
class ClientSession;
class LogService;

// Estrutura para manter os dados de um usu�rio online
struct UserSession {
    int accountId;
    std::string accountName;
    std::shared_ptr<ClientSession> session;
    // Adicione aqui outros dados, como o personagem selecionado, etc.
};

/**
 * @class UserService
 * @brief Gerencia os usu�rios atualmente online no servidor.
 */
class UserService {
public:
    explicit UserService(LogService& logService);

    /**
     * @brief Adiciona um novo usu�rio � lista de jogadores online.
     * @param accountId O ID da conta do usu�rio.
     * @param session A sess�o de rede do usu�rio.
     */
    void addUser(int accountId, std::shared_ptr<ClientSession> session);

    /**
     * @brief Remove um usu�rio da lista de jogadores online.
     * @param session A sess�o de rede do usu�rio a ser removido.
     */
    void removeUser(std::shared_ptr<ClientSession> session);

    /**
     * @brief Verifica se uma conta j� est� online.
     * @param accountId O ID da conta a ser verificado.
     * @return true se a conta estiver online, false caso contr�rio.
     */
    bool isUserOnline(int accountId);

private:
    LogService& m_logService;

    // Um mapa para armazenar as sess�es dos usu�rios online, usando o ID da conta como chave.
    // O std::mutex garante que o acesso ao mapa seja seguro entre m�ltiplas threads.
    std::map<int, UserSession> m_onlineUsers;
    std::mutex m_usersMutex;
};