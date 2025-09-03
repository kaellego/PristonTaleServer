#include "GameLogic/Services/AccountService.h"
#include "Database/DatabaseManager.h"
#include "Database/SQLConnection.h"
#include "LogService.h" // Supondo a exist�ncia de um LogService
#include "CharacterService.h" // Supondo a exist�ncia de um CharacterService
#include "UserService.h" // Supondo a exist�ncia de um UserService
#include "Network/ClientSession.h"

AccountService::AccountService(DatabaseManager& dbManager, LogService& logService, CharacterService& charService, UserService& userService)
    : m_dbManager(dbManager),
    m_logService(logService),
    m_characterService(charService),
    m_userService(userService)
{
    // Inicia a thread de trabalho que ficar� monitorando a fila de logins
    m_workerThread = std::thread(&AccountService::processLoginQueue, this);
}

AccountService::~AccountService() {
    // Sinaliza para a thread parar
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_isStopping = true;
    }
    m_condition.notify_one(); // Acorda a thread para que ela possa encerrar

    // Espera a thread terminar sua execu��o
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void AccountService::queueLoginRequest(LoginRequest request) {
    // Trava o mutex para adicionar um item � fila de forma segura
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_loginQueue.push(std::move(request));
    }
    // Notifica a thread de trabalho que h� um novo item na fila
    m_condition.notify_one();
}

void AccountService::processLoginQueue() {
    while (!m_isStopping) {
        LoginRequest request;
        {
            // Trava o mutex e espera por uma notifica��o (ou at� que a flag de parada seja true)
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this] { return !m_loginQueue.empty() || m_isStopping; });

            if (m_isStopping && m_loginQueue.empty()) {
                return; // Encerra a thread
            }

            // Pega o primeiro item da fila
            request = std::move(m_loginQueue.front());
            m_loginQueue.pop();
        }

        // Processa o item fora do lock para n�o prender a fila
        processSingleLogin(request);
    }
}

void AccountService::processSingleLogin(LoginRequest& request) {
    // --- L�GICA DE LOGIN REATORADA ---
    // Esta fun��o substitui a antiga `ProcessAccountLogin`

    // 1. Obter dados do usu�rio do banco de dados
    //    auto userInfo = m_characterService.getAccountInfo(request.accountName);

    // 2. Realizar uma s�rie de valida��es
    //    if (!userInfo.has_value()) {
    //        // Envia pacote de conta incorreta
    //        // m_logService.logEvent(Log::AccountEvent::IncorrectAccount, ...);
    //        return;
    //    }
    //    if (!validatePassword(request.password, userInfo->hashedPassword)) {
    //        // Envia pacote de senha incorreta
    //        // m_logService.logEvent(Log::AccountEvent::IncorrectPassword, ...);
    //        return;
    //    }
    //    // ... outras checagens (ban, manuten��o, etc.)

    // 3. Se tudo estiver OK:
    //    m_logService.logEvent(Log::AccountEvent::LoginSuccess, ...);

    //    // Associa a conta � sess�o do cliente
    //    // request.session->setAuthenticated(userInfo->accountId, userInfo->accountName);

    //    // Envia o pacote com a lista de personagens
    //    // auto charList = m_characterService.getCharacterList(userInfo->accountId);
    //    // request.session->send(createCharacterListPacket(charList));

    std::cout << "Processando login para: " << request.accountName << std::endl;
}

void AccountService::handleSelectCharacter(std::shared_ptr<ClientSession> session, const std::string& charName) {
    // ... L�gica para quando o jogador seleciona um personagem ...
}

// ... Implementa��o de outros m�todos, como as fun��es de verifica��o (checkBans, etc.)