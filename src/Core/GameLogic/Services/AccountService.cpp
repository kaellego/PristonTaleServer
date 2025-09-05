#include "GameLogic/Services/AccountService.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include "Network/ClientSession.h"
#include "Config/ServerConfig.h"
#include "Network/Packet.h"
#include "Utils/Crypto.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "Utils/Dice.h"

// TODO: Incluir os headers dos outros serviços quando eles forem criados
#include "GameLogic/Services/CharacterService.h" 
#include "GameLogic/Services/UserService.h"
#include "Logging/LogService.h"

#include <iostream>
#include <utility>

AccountService::AccountService(DatabasePool& dbPool, CharacterService& charService, UserService& userService, LogService& logService, const ServerConfig& serverConfig)
    : m_dbPool(dbPool),
    m_characterService(charService),
    m_userService(userService),
    m_logService(logService),
    m_serverConfig(serverConfig)
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
    std::string accountName(request.packet.szUserID);
    std::string passwordHashFromClient(request.packet.szPassword);

    m_logService.debug("Processando tentativa de login para a conta: '{}' com a senha: '{}'", accountName, passwordHashFromClient);

    // 1. Get user data from the database
    auto sqlUserOpt = getSqlUserInfo(accountName);
    if (!sqlUserOpt.has_value()) {
        m_logService.warn("Conta '{}' nao encontrada no banco de dados.", accountName);
        onLoginFailure(request.session, Log::LoginResult::IncorrectAccount, accountName);
        return;
    }

    SQLUser& sqlUser = *sqlUserOpt;

    // 2. Validate the request
    Log::LoginResult validationResult = validateRequest(request, sqlUser);
    if (validationResult != Log::LoginResult::Success) {
        m_logService.warn("Tentativa de login falhou para a conta {} com o codigo: {}", accountName, static_cast<int>(validationResult));
        onLoginFailure(request.session, validationResult, accountName);
        return;
    }

    // 3. Validate the password
    
    if (!Crypto::validatePassword(passwordHashFromClient, std::string(sqlUser.szPassword))) {
        m_logService.warn("Tentativa de login para a conta {} com hash de senha incorreto.", accountName);
        onLoginFailure(request.session, Log::LoginResult::IncorrectPassword, accountName);
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

void AccountService::sendServerList(std::shared_ptr<ClientSession> session, int ticket) {
    PacketServerList serverListPacket{};

    // As chamadas agora são válidas
    const auto& allServers = m_serverConfig.getAllServers();
    const auto& loginServerInfo = m_serverConfig.getThisServerInfo();

    serverListPacket.header.opcode = static_cast<uint32_t>(Opcodes::ServerList);
    // CORRIGIDO: strncpy_s precisa do tamanho do buffer de destino
    //strncpy_s(serverListPacket.szServerName, sizeof(serverListPacket.szServerName), loginServerInfo.name.c_str());
    serverListPacket.iTicket = ticket;
    serverListPacket.dwTime = GetTickCount();

    int gameServerCount = 0;
    for (const auto& serverInfo : allServers) {
        if (serverInfo.isGameServer && gameServerCount < 4) {
            auto& destServer = serverListPacket.sServers[gameServerCount];
            //strncpy_s(destServer.szName, sizeof(destServer.szName), serverInfo.name.c_str());
            //strncpy_s(destServer.szaIP[0], sizeof(destServer.szaIP[0]), serverInfo.publicIp.c_str());
            destServer.iaPort[0] = serverInfo.port;
            destServer.iaPort[1] = serverInfo.port;
            destServer.iaPort[2] = serverInfo.port;
            gameServerCount++;
        }
    }
    serverListPacket.iGameServers = gameServerCount;

    // Converte e envia o pacote
    const size_t packetSize = sizeof(PacketHeader) + offsetof(PacketServerList, sServers) - sizeof(PacketHeader) + (sizeof(ServerListServerInfo) * gameServerCount);
    serverListPacket.header.length = static_cast<uint16_t>(packetSize);

    const uint8_t* bodyPtr = reinterpret_cast<const uint8_t*>(&serverListPacket) + sizeof(PacketHeader);
    std::vector<uint8_t> body(bodyPtr, bodyPtr + (packetSize - sizeof(PacketHeader)));

    Packet packet(serverListPacket.header.opcode, body);

    m_logService.packet("-> Pacote Enviado: {} (Opcode: 0x{:04x})", "ServerList", packet.header.opcode);
    session->send(packet);
}

void AccountService::onLoginSuccess(std::shared_ptr<ClientSession> session, const SQLUser& sqlUser) {
    m_logService.info("Login bem-sucedido para a conta: {}", sqlUser.szAccountName);

    // CORRIGIDO: Gera o ticket e o passa para a sessão
    int ticket = Dice::RandomI(1, 1000000);
    session->authenticate(sqlUser.iID, sqlUser.szAccountName, ticket);

    m_userService.addUser(sqlUser.iID, sqlUser.szAccountName, session);

    sendLoginResult(session, Log::LoginResult::Success);
    sendServerList(session, ticket); // Passa o ticket para a lista de servidores
    sendCharacterList(session, sqlUser.szAccountName);
}

void AccountService::onLoginFailure(std::shared_ptr<ClientSession> session, Log::LoginResult reason, const std::string& accountName) {
    m_logService.warn("Login falhou para a conta [{}]. Razao: {}", accountName, static_cast<int>(reason));
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

void AccountService::sendCharacterList(std::shared_ptr<ClientSession> session, const std::string& accountName) {
    m_logService.info("Enviando lista de personagens para a conta {}", accountName);

    // Chama o CharacterService com o accountName
    std::vector<CharacterData> charList = m_characterService.getCharacterList(accountName);

    PacketUserInfo userInfoPacket{};
    userInfoPacket.header.opcode = static_cast<uint32_t>(Opcodes::UserInfo);
    strncpy_s(userInfoPacket.szUserID, accountName.c_str(), sizeof(userInfoPacket.szUserID));
    userInfoPacket.CharCount = static_cast<int>(charList.size());

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