#include "GameLogic/Services/AccountService.h"
#include "Database/DatabaseManager.h"
#include "Database/SQLConnection.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"
#include "Utils/Crypto.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"

// TODO: Incluir os headers dos outros serviços quando eles forem criados
// #include "GameLogic/Services/CharacterService.h" 
// #include "GameLogic/Services/UserService.h"
#include "Logging/LogService.h"

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

    // A lógica de validação agora usa os tipos corretos
    Log::LoginResult validationResult = validateRequest(request, sqlUser);
    if (validationResult != Log::LoginResult::Success) {
        m_logService.warn("Tentativa de login falhou para a conta {} com o codigo: {}", accountName, static_cast<int>(validationResult));
        onLoginFailure(request.session, validationResult);
        return;
    }

    if (!Crypto::validatePassword(password, std::string(sqlUser.szPassword))) {
        m_logService.warn("Tentativa de login para a conta {} com senha incorreta.", accountName);
        onLoginFailure(request.session, Log::LoginResult::IncorrectPassword);
        return;
    }

    onLoginSuccess(request.session, sqlUser);
}

Log::LoginResult AccountService::validateRequest(const LoginRequest& request, const SQLUser& sqlUser) {
    // CORRIGIDO: Agora a comparação direta funciona porque os tipos são os mesmos (EBanStatus == EBanStatus)
    if (sqlUser.iBanStatus == EBanStatus::BANSTATUS_Banned) {
        return Log::LoginResult::Banned;
    }
    if (sqlUser.iBanStatus == EBanStatus::BANSTATUS_TempBanned) {
        return Log::LoginResult::TempBanned;
    }

    return Log::LoginResult::Success;
}

void AccountService::onLoginSuccess(std::shared_ptr<ClientSession> session, const SQLUser& sqlUser) {
    m_logService.info("Login bem-sucedido para a conta: {}", sqlUser.szAccountName);
}

void AccountService::onLoginFailure(std::shared_ptr<ClientSession> session, Log::LoginResult reason) {
    sendLoginResult(session, reason);
}

std::optional<SQLUser> AccountService::getSqlUserInfo(const std::string& accountName) {
    SQLConnection* db = m_dbManager.getConnection(EDatabaseID::UserDB_Primary);
    if (!db) {
        m_logService.error("Nao foi possivel obter conexao com UserDB_Primary.");
        return std::nullopt;
    }

    try {
        db->prepare("SELECT TOP 1 ID, AccountName, Password, Flag, BanStatus, IsMuted FROM UserInfo WHERE AccountName=?");
        db->bindParameter<std::string>(1, accountName);

        // CORRIGIDO: A chamada de execute() não retorna valor, então não pode ser usada com &&
        db->execute();

        // A verificação é feita apenas no fetch()
        if (db->fetch()) {
            SQLUser user{};
            user.iID = db->getData<int>(1).value_or(0);

            auto dbAccountName = db->getData<std::string>(2).value_or("");
            strncpy_s(user.szAccountName, dbAccountName.c_str(), sizeof(user.szAccountName));

            auto dbPassword = db->getData<std::string>(3).value_or("");
            strncpy_s(user.szPassword, dbPassword.c_str(), sizeof(user.szPassword));

            user.iFlag = db->getData<int>(4).value_or(0);

            // CORRIGIDO: Atribui o int do DB para o enum class com um static_cast
            user.iBanStatus = static_cast<EBanStatus>(db->getData<int>(5).value_or(0));

            user.bIsMuted = db->getData<bool>(6).value_or(false);

            return user;
        }
    }
    catch (const std::exception& e) {
        m_logService.error("Erro de SQL em getSqlUserInfo para a conta {}: {}", accountName, e.what());
    }

    return std::nullopt;
}

void AccountService::sendLoginResult(std::shared_ptr<ClientSession> session, Log::LoginResult code, const std::string& message) {
    PacketAccountLoginCode responseStruct{};
    responseStruct.header.opcode = static_cast<uint16_t>(Opcodes::AccountLoginCode);
    responseStruct.iCode = static_cast<int>(code);
    strncpy_s(responseStruct.szMessage, message.c_str(), sizeof(responseStruct.szMessage) - 1);

    const size_t bodySize = sizeof(PacketAccountLoginCode) - sizeof(PacketHeader);
    const uint8_t* bodyPtr = reinterpret_cast<const uint8_t*>(&responseStruct) + sizeof(PacketHeader);
    std::vector<uint8_t> body(bodyPtr, bodyPtr + bodySize);

    Packet packet(responseStruct.header.opcode, body);
    session->send(packet);
}

void AccountService::sendCharacterList(std::shared_ptr<ClientSession> session, const std::string& accountName) {
    m_logService.info("Enviando lista de personagens para a conta {}", accountName);
    // TODO: Implementar a lógica de buscar os personagens no CharacterService
    // e montar o pacote PacketUserInfo para enviar ao cliente.
}
