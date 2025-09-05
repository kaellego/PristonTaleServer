#include "GameLogic/Services/AccountService.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"
#include "Utils/Crypto.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"

// TODO: Incluir os headers dos outros serviços quando eles forem criados
#include "GameLogic/Services/CharacterService.h" 
#include "GameLogic/Services/UserService.h"
#include "Logging/LogService.h"

#include <iostream>
#include <utility>

AccountService::AccountService(DatabasePool& dbPool, CharacterService& charService, UserService& userService, LogService& logService)
    : m_dbPool(dbPool),
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

    // --- A FORMA CORRETA E SIMPLES DE EXTRAIR AS STRINGS ---
    // O construtor de std::string sabe como lidar com arrays de char terminados em nulo.
    std::string accountName(request.packet.szUserID);
    std::string password(request.packet.szPassword);

    m_logService.debug("Processando tentativa de login para a conta: '{}' com a senha: '{}'", accountName, password);

    // 1. Get user data from the database
    auto sqlUserOpt = getSqlUserInfo(accountName);
    if (!sqlUserOpt.has_value()) {
        m_logService.warn("Conta '{}' nao encontrada no banco de dados.", accountName);
        onLoginFailure(request.session, Log::LoginResult::IncorrectAccount);
        return;
    }

    SQLUser& sqlUser = *sqlUserOpt;

    // 2. Validate the request
    Log::LoginResult validationResult = validateRequest(request, sqlUser);
    if (validationResult != Log::LoginResult::Success) {
        m_logService.warn("Tentativa de login falhou para a conta {} com o codigo: {}", accountName, static_cast<int>(validationResult));
        onLoginFailure(request.session, validationResult);
        return;
    }

    // 3. Validate the password
    if (!Crypto::validatePassword(accountName, password, std::string(sqlUser.szPassword))) {
        m_logService.warn("Tentativa de login para a conta {} com senha incorreta.", accountName);
        onLoginFailure(request.session, Log::LoginResult::IncorrectPassword);
        return;
    }

    // 4. Success!
    onLoginSuccess(request.session, sqlUser);
}

Log::LoginResult AccountService::validateRequest(const LoginRequest& request, const SQLUser& sqlUser) {
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
    session->authenticate(sqlUser.iID, sqlUser.szAccountName);
    m_userService.addUser(sqlUser.iID, sqlUser.szAccountName, session);
    sendLoginResult(session, Log::LoginResult::Success);
    sendCharacterList(session, sqlUser.szAccountName, sqlUser.iID);
}

void AccountService::onLoginFailure(std::shared_ptr<ClientSession> session, Log::LoginResult reason) {
    m_logService.warn("Login falhou para a conta [{}]. Razao: {}", session->getAccountName(), static_cast<int>(reason));
    sendLoginResult(session, reason);
}

std::optional<SQLUser> AccountService::getSqlUserInfo(const std::string& accountName) {
    auto db = m_dbPool.getConnection(EDatabaseID::UserDB_Primary);
    if (!db) {
        // Esta parte só será executada se, por algum motivo, o pool não puder
        // fornecer uma conexão válida (o que é improvável no design atual,
        // mas é uma excelente verificação de segurança).
        m_logService.error("Nao foi possivel obter uma conexao do pool para UserDB_Primary.");
        return std::nullopt; // Ou outra forma de tratamento de erro
    }
    try {
        db->prepare("SELECT TOP 1 ID, AccountName, Password, Flag, BanStatus, IsMuted FROM UserInfo WHERE AccountName=?");
        db->bindParameter<std::string>(1, accountName);
        db->execute();
        if (db->fetch()) {
            SQLUser user{};
            user.iID = db->getData<int>(1).value_or(0);
            auto dbAccountName = db->getData<std::string>(2).value_or("");
            strncpy_s(user.szAccountName, dbAccountName.c_str(), sizeof(user.szAccountName));
            auto dbPassword = db->getData<std::string>(3).value_or("");
            strncpy_s(user.szPassword, dbPassword.c_str(), sizeof(user.szPassword));
            user.iFlag = db->getData<int>(4).value_or(0);
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
    responseStruct.header.opcode = static_cast<uint32_t>(Opcodes::AccountLoginCode);
    responseStruct.iCode = static_cast<int>(code);
    strncpy_s(responseStruct.szMessage, message.c_str(), sizeof(responseStruct.szMessage) - 1);
    const size_t bodySize = sizeof(PacketAccountLoginCode) - sizeof(PacketHeader);
    const uint8_t* bodyPtr = reinterpret_cast<const uint8_t*>(&responseStruct) + sizeof(PacketHeader);
    std::vector<uint8_t> body(bodyPtr, bodyPtr + bodySize);
    Packet packet(responseStruct.header.opcode, body);
    m_logService.packet("-> Pacote Enviado: {} (Opcode: 0x{:04x})", getOpcodeName(packet.header.opcode), packet.header.opcode);
    session->send(packet);
}

void AccountService::sendCharacterList(std::shared_ptr<ClientSession> session, const std::string& accountName, int accountId) {
    m_logService.info("Enviando lista de personagens para a conta {}", accountName);

    std::vector<CharacterData> charList = m_characterService.getCharacterList(accountId);

    // Agora o compilador sabe exatamente o que é PacketUserInfo
    PacketUserInfo userInfoPacket{};
    userInfoPacket.header.opcode = PKTHDR_UserInfo;
    strncpy_s(userInfoPacket.szUserID, accountName.c_str(), sizeof(userInfoPacket.szUserID));
    userInfoPacket.CharCount = static_cast<int>(charList.size());

    // Preenche os dados de cada personagem no pacote
    for (size_t i = 0; i < charList.size() && i < 6; ++i) {
        const auto& charData = charList[i];
        auto& destChar = userInfoPacket.sCharacterData[i];

        // Copia os dados do CharacterData (do arquivo .chr) para a struct do pacote
        strncpy_s(destChar.szName, charData.szName, sizeof(destChar.szName));
        destChar.iLevel = charData.iLevel;
        destChar.JobCode = charData.iClass;
        // ... aqui você copiaria os outros campos necessários, como modelo 3D, mapa, etc.
    }

    // Converte e envia o pacote
    const size_t bodySize = sizeof(PacketUserInfo) - sizeof(PacketHeader);
    const uint8_t* bodyPtr = reinterpret_cast<const uint8_t*>(&userInfoPacket) + sizeof(PacketHeader);
    std::vector<uint8_t> body(bodyPtr, bodyPtr + bodySize);

    Packet packet(userInfoPacket.header.opcode, body);

    m_logService.packet("-> Pacote Enviado: {} (Opcode: 0x{:04x})", "UserInfo", packet.header.opcode);
    session->send(packet);
}