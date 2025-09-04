#include "Config/ServerConfig.h"
#include "Utils/INIReader.h"

#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>

ServerConfig::ServerConfig(const std::string& filename) {
    if (!std::filesystem::exists(filename)) {
        std::cout << "AVISO: Arquivo '" << filename << "' nao encontrado. Gerando um arquivo padrao." << std::endl;
        generateDefaultConfig(filename);
    }

    INIReader reader(filename);
    if (reader.ParseError() != 0) {
        throw std::runtime_error("Nao foi possivel analisar o arquivo de configuracao: " + filename);
    }
    loadAndValidate(reader);
    calculateKeys();
}

void ServerConfig::calculateKeys() {
    // Tradução direta da macro #define GAME_XOR
    m_xorKey = ((((m_gameVersion) * 519 * 16510 * 14 + 595) % 180) + 20);

    // Log para podermos verificar se o cálculo está correto
    std::cout << "[INFO] Versao do Jogo: " << m_gameVersion << std::endl;
    std::cout << "[INFO] Chave XOR Calculada: 0x" << std::hex << static_cast<int>(m_xorKey) << std::dec << std::endl;
}

void ServerConfig::generateDefaultConfig(const std::string& filename) const {
    std::ofstream configFile(filename);
    if (!configFile.is_open()) {
        throw std::runtime_error("Nao foi possivel criar o arquivo: " + filename);
    }

    configFile << "; Arquivo de configuracao do Servidor Priston Tale (Gerado Automaticamente)" << std::endl;
    configFile << std::endl;

    configFile << "[Server]" << std::endl;
    configFile << "Version = 1309" << std::endl;
    configFile << "GameServers = 1" << std::endl;
    configFile << "Type = GameServer1" << std::endl;
    configFile << "MaxUsers = 550" << std::endl << std::endl;

    configFile << "[LoginServer]" << std::endl;
    configFile << "ID = 0" << std::endl;
    configFile << "Name = Beta" << std::endl;
    configFile << "IP = 127.0.0.1" << std::endl;
    configFile << "NetIP = 127.0.0.1" << std::endl;
    configFile << "Port = 5055" << std::endl;
    configFile << "MaxUsers = 550" << std::endl << std::endl;

    configFile << "[GameServer1]" << std::endl;
    configFile << "ID = 1" << std::endl;
    configFile << "Name = Awell" << std::endl;
    configFile << "IP = 127.0.0.1" << std::endl;
    configFile << "Port = 5056" << std::endl;
    configFile << "MaxUsers = 550" << std::endl << std::endl;

    configFile << "[Database]" << std::endl;
    configFile << "Driver = {ODBC Driver 17 for SQL Server}" << std::endl;
    configFile << "Host = localhost" << std::endl;
    configFile << "User = sa" << std::endl;
    configFile << "Password = YourStrong(!)Password" << std::endl;
    configFile << "NetFolder = ./data/items" << std::endl;

    configFile.close();
}

void ServerConfig::loadAndValidate(INIReader& reader) {
    // Carrega [Server]
    m_gameVersion = reader.GetInteger("Server", "Version", 1309);
    std::string serverType = reader.Get("Server", "Type", "GameServer1");

    // Carrega a lista de servidores
    loadServerList(reader);

    // Encontra o ID do servidor atual
    m_serverCode = reader.GetInteger(serverType, "ID", 1);

    // Carrega [Database]
    m_databaseConfig.driver = reader.Get("Database", "Driver", "{ODBC Driver 17 for SQL Server}");
    m_databaseConfig.host = reader.Get("Database", "Host", "");
    m_databaseConfig.user = reader.Get("Database", "User", "");
    m_databaseConfig.password = reader.Get("Database", "Password", "");
    m_databaseConfig.netFolder = reader.Get("Database", "NetFolder", "");

    // Validação
    if (m_databaseConfig.host.empty() || m_databaseConfig.user.empty()) {
        throw std::runtime_error("Host e User do banco de dados nao podem estar vazios no server.ini.");
    }
}

void ServerConfig::loadServerList(INIReader& reader) {
    m_allServers.clear();

    // Carrega o Login Server
    ServerInfo loginServer;
    loginServer.name = reader.Get("LoginServer", "Name", "Login Server");
    loginServer.publicIp = reader.Get("LoginServer", "IP", "127.0.0.1");
    loginServer.networkIp = reader.Get("LoginServer", "NetIP", "127.0.0.1");
    loginServer.port = static_cast<uint16_t>(reader.GetInteger("LoginServer", "Port", 5055));
    loginServer.maxUsers = reader.GetInteger("LoginServer", "MaxUsers", 500);
    loginServer.isLoginServer = true;
    m_allServers.push_back(loginServer);

    // Carrega os Game Servers
    int gameServerCount = reader.GetInteger("Server", "GameServers", 1);
    for (int i = 1; i <= gameServerCount; ++i) {
        std::string section = "GameServer" + std::to_string(i);
        ServerInfo gameServer;
        gameServer.name = reader.Get(section, "Name", "Game Server " + std::to_string(i));
        gameServer.publicIp = reader.Get(section, "IP", "127.0.0.1");
        gameServer.port = static_cast<uint16_t>(reader.GetInteger(section, "Port", 5056 + i - 1));
        gameServer.maxUsers = reader.GetInteger(section, "MaxUsers", 500);
        gameServer.isGameServer = true;
        m_allServers.push_back(gameServer);
    }
}

const ServerInfo& ServerConfig::getThisServerInfo() const {
    if (m_serverCode >= 0 && static_cast<size_t>(m_serverCode) < m_allServers.size()) {
        return m_allServers[m_serverCode];
    }
    throw std::runtime_error("Server 'Type' ou 'ID' invalidos no server.ini.");
}

const GlobalDatabaseConfig& ServerConfig::getDatabaseConfig() const {
    return m_databaseConfig;
}