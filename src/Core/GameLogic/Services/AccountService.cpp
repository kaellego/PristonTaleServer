#include "GameLogic/Services/AccountService.h"
#include "Database/DatabaseManager.h"
#include "Database/SQLConnection.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"

// TODO: Incluir os headers dos outros serviços quando eles forem criados
// #include "GameLogic/Services/CharacterService.h" 
// #include "GameLogic/Services/UserService.h"
// #include "Logging/LogService.h"

#include <iostream>
#include <utility>

AccountService::AccountService(DatabaseManager& dbManager, CharacterService& charService, UserService& userService, LogService& logService)
    : m_dbManager(dbManager),
    m_characterService(charService),
    m_userService(userService),
    m_logService(logService)
{
    m_workerThread = std::thread(&AccountService::workerLoop, this);
}

AccountService::~AccountService() {
    m_isStopping = true;
    m_condition.notify_one();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void AccountService::queueLoginRequest(std::shared_ptr<ClientSession> session, const PacketLoginUser& packet) {
    LoginRequest request;
    request.session = session;
    request.packet = packet;
    try {
        request.ipAddress = session->socket().remote_endpoint().address().to_string();
    }
    catch (...) {
        request.ipAddress = "?.?.?.?";
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_loginQueue.push(std::move(request));
    }
    m_condition.notify_one();
}

void AccountService::workerLoop() {
    while (!m_isStopping) {
        LoginRequest request;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this] { return !m_loginQueue.empty() || m_isStopping; });

            if (m_isStopping && m_loginQueue.empty()) {
                return;
            }

            request = std::move(m_loginQueue.front());
            m_loginQueue.pop();
        }
        processLogin(request);
    }
}

void AccountService::processLogin(LoginRequest& request) {
    std::string accountName(request.packet.szUserID, strnlen_s(request.packet.szUserID, sizeof(request.packet.szUserID)));
    std::string password(request.packet.szPassword, strnlen_s(request.packet.szPassword, sizeof(request.packet.szPassword)));

    auto sqlUserOpt = getSqlUserInfo(accountName);

    if (!sqlUserOpt.has_value()) {
        sendLoginResult(request.session, Log::LoginResult::IncorrectAccount);
        return;
    }

    SQLUser& sqlUser = *sqlUserOpt;

    if (sqlUser.iBanStatus == static_cast<int>(EBanStatus::BANSTATUS_Banned)) {
        sendLoginResult(request.session, Log::LoginResult::Banned);
        return;
    }

    if (password != std::string(sqlUser.szPassword)) {
        sendLoginResult(request.session, Log::LoginResult::IncorrectPassword);
        return;
    }

    std::cout << "Login bem-sucedido para a conta: " << accountName << std::endl;
}

std::optional<SQLUser> AccountService::getSqlUserInfo(const std::string& accountName) {
    // Implementação do banco de dados...
    return std::nullopt; // Retornando vazio por enquanto para compilar
}

void AccountService::sendLoginResult(std::shared_ptr<ClientSession> session, Log::LoginResult code, const std::string& message) {
    PacketAccountLoginCode responseStruct;
    responseStruct.header.opcode = PKTHDR_AccountLoginCode;
    responseStruct.iCode = static_cast<int>(code);
    strncpy_s(responseStruct.szMessage, message.c_str(), sizeof(responseStruct.szMessage) - 1);

    // Calcula o tamanho do corpo (toda a struct menos o cabeçalho)
    const size_t bodySize = sizeof(PacketAccountLoginCode) - sizeof(PacketHeader);

    // Cria um ponteiro para o início do corpo
    const uint8_t* bodyPtr = reinterpret_cast<const uint8_t*>(&responseStruct) + sizeof(PacketHeader);

    // Cria um std::vector a partir do ponteiro e tamanho do corpo
    std::vector<uint8_t> body(bodyPtr, bodyPtr + bodySize);

    // CORRIGIDO: O construtor do Packet é (opcode, body)
    Packet packet(responseStruct.header.opcode, body);

    session->send(packet);
}
