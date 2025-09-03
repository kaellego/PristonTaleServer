#include "Config/ServerConfig.h"
#include "Utils/INIReader.h"

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

// Construtor principal: orquestra o carregamento, geração e validação.
ServerConfig::ServerConfig(const std::string& filename) {
    // 1. VERIFICAÇÃO: Checa se o arquivo de configuração existe.
    if (!std::filesystem::exists(filename)) {
        std::cout << "AVISO: Arquivo '" << filename << "' nao encontrado. Gerando um arquivo padrao." << std::endl;
        try {
            // 2. GERAÇÃO: Se não existir, cria um arquivo padrão.
            generateDefaultConfig(filename);
            std::cout << "INFO: Arquivo '" << filename << "' gerado com sucesso." << std::endl;
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Falha critica ao gerar o arquivo de configuracao padrao: " + std::string(e.what()));
        }
    }

    // 3. LEITURA: Procede com a leitura do arquivo.
    INIReader reader(filename);
    if (reader.ParseError() != 0) {
        throw std::runtime_error("Nao foi possivel carregar ou analisar o arquivo de configuracao: " + filename);
    }

    loadGeneralSettings(reader);
    loadServerList(reader);
    loadEventSettings(reader);
    loadDatabaseSettings(reader);

    // 4. VALIDAÇÃO: Garante que as configurações carregadas são válidas.
    validateConfig();
}

void ServerConfig::generateDefaultConfig(const std::string& filename) const {
    std::ofstream configFile(filename);
    if (!configFile.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o arquivo para escrita: " + filename);
    }

    // Escreve o conteúdo padrão no arquivo com comentários
    configFile << "; Arquivo de configuracao do Servidor Priston Tale (Gerado Automaticamente)" << std::endl;
    configFile << "; Data de geracao: " << __DATE__ << std::endl << std::endl;

    configFile << "[Version]" << std::endl;
    configFile << "Game = 1610" << std::endl << std::endl;

    configFile << "[Server]" << std::endl;
    configFile << "Type = LoginServer         ; O tipo deste executavel. Pode ser LoginServer, GameServer1, etc." << std::endl;
    configFile << "GameServers = 1            ; O numero de GameServers listados abaixo." << std::endl;
    configFile << "Seasonal = false           ; Define se este e um servidor de temporada." << std::endl << std::endl;

    configFile << "[LoginServer]" << std::endl;
    configFile << "ID = 0                     ; ID unico para este servidor (0 para LoginServer)." << std::endl;
    configFile << "Name = Priston Tale        ; Nome do servidor que aparece no cliente." << std::endl;
    configFile << "IP = 127.0.0.1             ; IP publico para os jogadores se conectarem." << std::endl;
    configFile << "NetIP = 127.0.0.1          ; IP para comunicacao entre servidores (pode ser um IP de rede local)." << std::endl;
    configFile << "Port = 5055                ; Porta de escuta." << std::endl;
    configFile << "MaxUsers = 500" << std::endl << std::endl;

    configFile << "[GameServer1]" << std::endl;
    configFile << "ID = 1" << std::endl;
    configFile << "Name = Awell" << std::endl;
    configFile << "IP = 127.0.0.1" << std::endl;
    configFile << "NetIP = 127.0.0.1" << std::endl;
    configFile << "Port = 5056" << std::endl;
    configFile << "MaxUsers = 500" << std::endl << std::endl;

    configFile << "[Event]" << std::endl;
    configFile << "RateExp = 1" << std::endl;
    configFile << "SiegeWar = false" << std::endl;
    configFile << "SiegeWarDay = 7              ; 1=Seg, 2=Ter, ..., 7=Dom" << std::endl;
    configFile << "SiegeWarHour = 20            ; Hora do dia (formato 24h)" << std::endl;
    configFile << "Mimic = false" << std::endl;
    configFile << "Halloween = false" << std::endl;
    configFile << "HalloweenPercent = 5" << std::endl;
    configFile << "Christmas = false" << std::endl << std::endl;

    configFile << "[Database]" << std::endl;
    configFile << "; Liste os nomes de todas as suas conexoes de banco de dados, separados por virgula." << std::endl;
    configFile << "Connections = GameDB,LogDB,UserDB" << std::endl << std::endl;

    configFile << "; Configure os detalhes de cada conexao listada acima." << std::endl;
    configFile << "[Database_GameDB]" << std::endl;
    configFile << "Host = 127.0.0.1" << std::endl;
    configFile << "Port = 1433" << std::endl;
    configFile << "User = sa" << std::endl;
    configFile << "Password = YourPasswordHere" << std::endl;
    configFile << "Database = GameDB" << std::endl << std::endl;

    configFile << "[Database_LogDB]" << std::endl;
    configFile << "Host = 127.0.0.1" << std::endl;
    configFile << "Port = 1433" << std::endl;
    configFile << "User = sa" << std::endl;
    configFile << "Password = YourPasswordHere" << std::endl;
    configFile << "Database = LogDB" << std::endl << std::endl;

    configFile << "[Database_UserDB]" << std::endl;
    configFile << "Host = 127.0.0.1" << std::endl;
    configFile << "Port = 1433" << std::endl;
    configFile << "User = sa" << std::endl;
    configFile << "Password = YourPasswordHere" << std::endl;
    configFile << "Database = UserDB" << std::endl << std::endl;

    configFile.close();
}

void ServerConfig::loadGeneralSettings(INIReader& reader) {
    m_gameVersion = reader.GetInteger("Version", "Game", 1610);
    m_isSeasonal = reader.GetBoolean("Server", "Seasonal", false);
    std::string serverType = reader.Get("Server", "Type", "LoginServer");
    m_serverCode = reader.GetInteger(serverType, "ID", 0);
}

void ServerConfig::loadServerList(INIReader& reader) {
    ServerInfo loginServer;
    loginServer.name = reader.Get("LoginServer", "Name", "Login Server");
    loginServer.publicIp = reader.Get("LoginServer", "IP", "127.0.0.1");
    loginServer.networkIp = reader.Get("LoginServer", "NetIP", "127.0.0.1");
    loginServer.port = static_cast<uint16_t>(reader.GetInteger("LoginServer", "Port", 5055));
    loginServer.maxUsers = reader.GetInteger("LoginServer", "MaxUsers", 500);
    loginServer.isLoginServer = true;
    m_allServers.push_back(loginServer);

    int gameServerCount = reader.GetInteger("Server", "GameServers", 1);
    for (int i = 1; i <= gameServerCount; ++i) {
        std::string section = "GameServer" + std::to_string(i);
        ServerInfo gameServer;
        gameServer.name = reader.Get(section, "Name", "Game Server " + std::to_string(i));
        gameServer.publicIp = reader.Get(section, "IP", "127.0.0.1");
        gameServer.networkIp = reader.Get(section, "NetIP", "127.0.0.1");
        gameServer.port = static_cast<uint16_t>(reader.GetInteger(section, "Port", 5056 + i - 1));
        gameServer.maxUsers = reader.GetInteger(section, "MaxUsers", 500);
        gameServer.isGameServer = true;
        m_allServers.push_back(gameServer);
    }
}

void ServerConfig::loadEventSettings(INIReader& reader) {
    m_eventSettings.rateExp = reader.GetInteger("Event", "RateExp", 1);
    m_eventSettings.siegeWarEnabled = reader.GetBoolean("Event", "SiegeWar", false);
    m_eventSettings.siegeWarDay = reader.GetInteger("Event", "SiegeWarDay", 7);
    m_eventSettings.siegeWarHour = reader.GetInteger("Event", "SiegeWarHour", 20);
    m_eventSettings.mimicEvent = reader.GetBoolean("Event", "Mimic", false);
    m_eventSettings.halloweenEvent = reader.GetBoolean("Event", "Halloween", false);
    m_eventSettings.halloweenRate = reader.GetInteger("Event", "HalloweenPercent", 5);
    m_eventSettings.christmasEvent = reader.GetBoolean("Event", "Christmas", false);
}

void ServerConfig::loadDatabaseSettings(INIReader& reader) {
    std::string connNames = reader.Get("Database", "Connections", "");
    if (connNames.empty()) return;

    std::string currentName;
    std::stringstream ss(connNames);
    while (std::getline(ss, currentName, ',')) {
        if (currentName.empty()) continue;
        std::string section = "Database_" + currentName;
        DatabaseConfig dbConf;
        dbConf.name = currentName;
        dbConf.host = reader.Get(section, "Host", "127.0.0.1");
        dbConf.user = reader.Get(section, "User", "sa");
        dbConf.password = reader.Get(section, "Password", "");
        dbConf.database = reader.Get(section, "Database", "");
        dbConf.port = static_cast<uint16_t>(reader.GetInteger(section, "Port", 1433));
        m_databaseConfigs.push_back(dbConf);
    }
}

void ServerConfig::validateConfig() const {
    if (m_allServers.empty()) {
        throw std::runtime_error("Validacao falhou: Nenhuma definicao de servidor (LoginServer/GameServer) encontrada no server.ini.");
    }
    if (m_databaseConfigs.empty()) {
        throw std::runtime_error("Validacao falhou: Nenhuma conexao de banco de dados configurada no server.ini.");
    }
    try {
        getThisServerInfo();
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Validacao falhou: Server 'Type' ou 'ID' invalidos no server.ini. Erro: " + std::string(e.what()));
    }
}

// Implementações dos Getters (sem duplicatas)
const ServerInfo& ServerConfig::getThisServerInfo() const {
    if (m_serverCode >= 0 && static_cast<size_t>(m_serverCode) < m_allServers.size()) {
        return m_allServers[m_serverCode];
    }
    throw std::runtime_error("ServerCode invalido: " + std::to_string(m_serverCode));
}

int ServerConfig::getServerCode() const { return m_serverCode; }
int ServerConfig::getGameVersion() const { return m_gameVersion; }
bool ServerConfig::isSeasonal() const { return m_isSeasonal; }
const std::vector<ServerInfo>& ServerConfig::getAllServers() const { return m_allServers; }
const EventSettings& ServerConfig::getEventSettings() const { return m_eventSettings; }
const std::vector<DatabaseConfig>& ServerConfig::getDatabaseConfigs() const { return m_databaseConfigs; }