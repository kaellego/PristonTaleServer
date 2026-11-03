#include "GameLogic/Services/AccountService.h"
#include "Utils/SafeString.h"
#include "Database/DatabasePool.h"
#include "Network/ClientSession.h"
#include "Config/ServerConfig.h"
#include "Network/Packet.h"
#include "Utils/Crypto.h"
#include "Utils/PasswordVerifier.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "Utils/Dice.h"

// TODO: Incluir os headers dos outros serviï¿½os quando eles forem criados
#include "GameLogic/Services/CharacterService.h" 
#include "GameLogic/Services/UserService.h"
#include "Logging/LogService.h"
#include "Database/Orm.h"
#include "Utils/picosha2.h"

#include <iostream>
#include <utility>
#include <array>
#include <algorithm>

namespace {
const char* modelBodyForClass(int characterClass) {
    static constexpr std::array<const char*, 11> names{
        "char\\tmABCD\\b001.ini", "char\\tmABCD\\a001.ini",
        "char\\tmABCD\\d001.ini", "char\\tmABCD\\c001.ini",
        "char\\tmABCD\\ma001.ini", "char\\tmABCD\\mb001.ini",
        "char\\tmABCD\\mc001.ini", "char\\tmABCD\\md001.ini",
        "char\\tmABCD\\e001.ini", "char\\tmABCD\\me001.ini",
        "char\\tmABCD\\f001.ini"};
    return characterClass >= 1 && characterClass <= static_cast<int>(names.size())
        ? names[static_cast<size_t>(characterClass - 1)] : names[0];
}

const char* modelFaceForClass(int characterClass) {
    static constexpr std::array<const char*, 11> names{
        "char\\tmABCD\\tmh-b01.inf", "char\\tmABCD\\tmh-a01.inf",
        "char\\tmABCD\\tfh-D01.inf", "char\\tmABCD\\tmh-c01.inf",
        "char\\tmABCD\\Mmh-A01.inf", "char\\tmABCD\\Mfh-B01.inf",
        "char\\tmABCD\\Mfh-C01.inf", "char\\tmABCD\\Mmh-D01.inf",
        "char\\tmABCD\\tfh-e01.inf", "char\\tmABCD\\Mmh-e01.inf",
        "char\\tmABCD\\tfh-f01.inf"};
    return characterClass >= 1 && characterClass <= static_cast<int>(names.size())
        ? names[static_cast<size_t>(characterClass - 1)] : names[0];
}

uint32_t legacyJobCodeForClass(int characterClass) {
    static constexpr uint32_t codes[12] = {0, 1, 2, 3, 4, 6, 5, 8, 7, 9, 10, 11};
    return characterClass >= 1 && characterClass <= 11 ? codes[characterClass] : codes[1];
}
}

AccountService::AccountService(DatabasePool& dbPool, CharacterService& charService, UserService& userService, LogService& logService, const ServerConfig& serverConfig)
    : m_dbPool(dbPool),
    m_characterService(charService),
    m_userService(userService),
    m_logService(logService),
    m_serverConfig(serverConfig)
{
    constexpr size_t workerCount = 2;
    m_workerThreads.reserve(workerCount);
    for (size_t i = 0; i < workerCount; ++i) {
        m_workerThreads.emplace_back(&AccountService::workerLoop, this);
    }
}

AccountService::~AccountService() {
    m_isStopping = true;
    m_condition.notify_all();
    for (auto& worker : m_workerThreads) {
        if (worker.joinable()) worker.join();
    }
}

bool AccountService::allowLoginAttempt(const std::string& ipAddress) {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard lock(m_rateMutex);
    auto& window = m_rateByIp[ipAddress];
    if (window.started.time_since_epoch().count() == 0 || now - window.started >= RateWindowSize) {
        window = {now, 0};
    }
    if (window.attempts >= MaxAttemptsPerMinute) return false;
    ++window.attempts;
    return true;
}

int AccountService::issueTicket(const std::string& accountName, const std::string& ipAddress, std::chrono::steady_clock::time_point expiresAt) {
    const int ticket = Dice::RandomI(1, 1000000);
    const auto expiry = expiresAt.time_since_epoch().count();
    const auto signature = picosha2::hash256_hex_string(accountName + "|" + ipAddress + "|" + std::to_string(ticket) + "|" + std::to_string(expiry));
    m_logService.debug("Ticket assinado emitido para '{}' (assinatura {}...).", accountName, signature.substr(0, 12));
    return ticket;
}

bool AccountService::queueLoginRequest(std::shared_ptr<ClientSession> session, const PacketLoginUser& packet) {
    LoginRequest request;
    request.session = session;
    request.packet = packet;
    try {
        request.ipAddress = session->socket().remote_endpoint().address().to_string();
    }

    catch (...) {
        request.ipAddress = "?.?.?.?";
    }

    if (!allowLoginAttempt(request.ipAddress)) {
        m_logService.warn("Rate limit de login atingido para {}.", request.ipAddress);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_isStopping || m_loginQueue.size() >= MaxLoginQueueSize) {
            m_logService.warn("Fila de login cheia; requisicao rejeitada para {}.", request.ipAddress);
            return false;
        }
        m_loginQueue.push(std::move(request));
    }
    m_condition.notify_one();
    return true;
}

void AccountService::workerLoop() {
    while (true) {
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
    std::string passwordHashFromClient(request.packet.szPassword, strnlen_s(request.packet.szPassword, sizeof(request.packet.szPassword)));
    if (accountName.empty() || accountName.size() >= sizeof(request.packet.szUserID)) {
        onLoginFailure(request.session, Log::LoginResult::IncorrectAccount, "");
        return;
    }

    m_logService.debug("Processando tentativa de login para a conta: '{}'", accountName);

    // 1. Get user data from the database
    auto sqlUserOpt = getSqlUserInfo(accountName);
    if (!sqlUserOpt.has_value()) {
        m_logService.warn("Conta '{}' nao encontrada no banco de dados.", accountName);
        onLoginFailure(request.session, Log::LoginResult::IncorrectAccount, accountName);
        return;
    }

    SQLUser& sqlUser = *sqlUserOpt;

    const size_t storedPasswordLength = strnlen_s(sqlUser.szPassword, sizeof(sqlUser.szPassword));
    m_logService.debug("Credencial recebida para '{}': tamanho={}, cadastro={}",
                       accountName, passwordHashFromClient.size(), storedPasswordLength);

    // 2. Validate the request
    Log::LoginResult validationResult = validateRequest(request, sqlUser);
    if (validationResult != Log::LoginResult::Success) {
        m_logService.warn("Tentativa de login falhou para a conta {} com o codigo: {}", accountName, static_cast<int>(validationResult));
        onLoginFailure(request.session, validationResult, accountName);
        return;
    }

    // 3. Validate the password
    
    if (!PasswordVerifier::matches(passwordHashFromClient, std::string(sqlUser.szPassword))) {
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

    // As chamadas agora sï¿½o vï¿½lidas
    const auto& allServers = m_serverConfig.getAllServers();
    const auto& loginServerInfo = m_serverConfig.getThisServerInfo();

    serverListPacket.header.opcode = static_cast<uint32_t>(Opcodes::ServerList);
    copyFixed(serverListPacket.szServerName, loginServerInfo.name);
    serverListPacket.iTicket = ticket;
    serverListPacket.dwTime = GetTickCount();

    int gameServerCount = 0;
    constexpr int maxGameServers = static_cast<int>(sizeof(serverListPacket.sServers) /
                                                    sizeof(serverListPacket.sServers[0]));
    for (const auto& serverInfo : allServers) {
        if (serverInfo.isGameServer && gameServerCount < maxGameServers) {
            auto& destServer = serverListPacket.sServers[gameServerCount];
            copyFixed(destServer.szName, serverInfo.name);
            copyFixed(destServer.szaIP[0], serverInfo.publicIp);
            copyFixed(destServer.szaIP[1], serverInfo.networkIp);
            copyFixed(destServer.szaIP[2], serverInfo.publicIp);
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

    // CORRIGIDO: Gera o ticket e o passa para a sessï¿½o
    const auto expiresAt = std::chrono::steady_clock::now() + TicketLifetime;
    std::string ipAddress = "unknown";
    try { ipAddress = session->socket().remote_endpoint().address().to_string(); } catch (...) {}
    int ticket = issueTicket(sqlUser.szAccountName, ipAddress, expiresAt);
    m_userService.addUser(sqlUser.iID, sqlUser.szAccountName, session);
    session->authenticate(sqlUser.iID, sqlUser.szAccountName, ticket, expiresAt);

    sendLoginResult(session, Log::LoginResult::Success);
    // The legacy SrcServer sends characters before the server list.
    sendCharacterList(session, sqlUser.szAccountName);
    sendServerList(session, ticket); // Passa o ticket para a lista de servidores
}

void AccountService::onLoginFailure(std::shared_ptr<ClientSession> session, Log::LoginResult reason, const std::string& accountName) {
    m_logService.warn("Login falhou para a conta [{}]. Razao: {}", accountName, static_cast<int>(reason));
    sendLoginResult(session, reason);
}

std::optional<SQLUser> AccountService::getSqlUserInfo(const std::string& accountName) {
    try {
        Orm orm(m_dbPool, m_logService, m_dbPool.dialect());
        return orm.findUser(accountName);
    } catch (const std::exception& e) {
        m_logService.error("Erro de ORM ao consultar a conta {}: {}", accountName, e.what());
        return std::nullopt;
    }
}
void AccountService::sendLoginResult(std::shared_ptr<ClientSession> session, Log::LoginResult code, const std::string& message) {
    PacketAccountLoginCode responseStruct{};
    responseStruct.header.opcode = static_cast<uint32_t>(Opcodes::AccountLoginCode);
    responseStruct.iCode = static_cast<int>(code);
    copyFixed(responseStruct.szMessage, message);
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
    copyFixed(userInfoPacket.szUserID, accountName);
    userInfoPacket.CharCount = static_cast<int>(std::min<size_t>(charList.size(), 6));

    for (size_t i = 0; i < charList.size() && i < 6; ++i) {
        const auto& charData = charList[i];
        auto& destChar = userInfoPacket.sCharacterData[i];

        // Copia os dados do CharacterData (do arquivo .chr) para a struct do pacote
        copyFixed(destChar.szName, charData.szName);
        destChar.iLevel = charData.iLevel;
        destChar.JobCode = legacyJobCodeForClass(charData.iClass);
        copyFixed(destChar.szModelName, modelBodyForClass(charData.iClass));
        copyFixed(destChar.szModelName2, modelFaceForClass(charData.iClass));
        destChar.Brood = 0;
        destChar.dwArmorCode = 0;
        destChar.StartField = charData.iMapId;
        destChar.PosX = static_cast<int32_t>(charData.fPosX);
        destChar.PosZ = static_cast<int32_t>(charData.fPosZ);
        std::fill(std::begin(destChar.dwTemp), std::end(destChar.dwTemp), 0);
        // ... aqui vocï¿½ copiaria os outros campos necessï¿½rios, como modelo 3D, mapa, etc.
    }

    // Converte e envia o pacote
    const size_t bodySize = sizeof(PacketUserInfo) - sizeof(PacketHeader);
    const uint8_t* bodyPtr = reinterpret_cast<const uint8_t*>(&userInfoPacket) + sizeof(PacketHeader);
    std::vector<uint8_t> body(bodyPtr, bodyPtr + bodySize);

    Packet packet(userInfoPacket.header.opcode, body);

    m_logService.packet("-> Pacote Enviado: {} (Opcode: 0x{:04x})", "UserInfo", packet.header.opcode);
    session->send(packet);
}
