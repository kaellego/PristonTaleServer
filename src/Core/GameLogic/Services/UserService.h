#pragma once

#include <string>
#include <map>
#include <mutex>
#include <memory>
#include "Shared/datatypes.h"

class ClientSession;
class LogService;

// Struct para manter os dados de um usu�rio online
struct UserSession {
    int accountId;
    std::string accountName; // <-- Adicionado
    std::shared_ptr<ClientSession> session;
};

class UserService {
public:
    explicit UserService(LogService& logService);

    // CORRIGIDO: Assinatura do m�todo addUser atualizada
    void addUser(int accountId, const std::string& accountName, std::shared_ptr<ClientSession> session);

    void removeUser(std::shared_ptr<ClientSession> session);
    bool isUserOnline(const std::string& accountName); // <-- Modificado para buscar por nome

private:
    LogService& m_logService;

    // A chave do mapa agora � o nome da conta (string), que � mais f�cil de usar.
    std::map<std::string, UserSession> m_onlineUsers;
    std::mutex m_usersMutex;
};