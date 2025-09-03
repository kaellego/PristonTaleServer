#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem> // Requer C++17 ou superior

// Forward declaration para a classe de leitura de INI para manter o header limpo
class INIReader;

// --- Estruturas de Dados para Configura��o ---

// Estrutura para armazenar informa��es de um �nico servidor (Login ou Game)
struct ServerInfo {
    std::string name;
    std::string publicIp;
    std::string networkIp;
    uint16_t    port;
    int         maxUsers;
    bool        isLoginServer = false;
    bool        isGameServer = false;
};

// Estrutura para as configura��es de eventos do jogo
struct EventSettings {
    int  rateExp = 1;
    bool siegeWarEnabled = false;
    int  siegeWarDay = 0;
    int  siegeWarHour = 0;
    bool mimicEvent = false;
    bool halloweenEvent = false;
    int  halloweenRate = 0;
    bool christmasEvent = false;
};

// Estrutura para as configura��es de banco de dados
struct DatabaseConfig {
    std::string name;
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    uint16_t    port;
};


/**
 * @class ServerConfig
 * @brief Carrega, valida e fornece acesso �s configura��es do server.ini.
 *
 * Esta classe tamb�m � capaz de gerar um arquivo server.ini padr�o
 * se ele n�o for encontrado na inicializa��o.
 */
class ServerConfig {
public:
    /**
     * @brief Construtor que carrega e analisa o arquivo .ini.
     * @param filename O caminho para o arquivo server.ini.
     */
    explicit ServerConfig(const std::string& filename);

    // --- Acessores P�blicos (Getters) ---
    int getServerCode() const;
    int getGameVersion() const;
    bool isSeasonal() const;
    const ServerInfo& getThisServerInfo() const;
    const std::vector<ServerInfo>& getAllServers() const;
    const EventSettings& getEventSettings() const;
    const std::vector<DatabaseConfig>& getDatabaseConfigs() const;

private:
    // --- M�todos de L�gica Interna ---
    void loadGeneralSettings(INIReader& reader);
    void loadServerList(INIReader& reader);
    void loadEventSettings(INIReader& reader);
    void loadDatabaseSettings(INIReader& reader);
    void validateConfig() const;
    void generateDefaultConfig(const std::string& filename) const;

    // --- Membros Privados ---
    int m_serverCode = 0;
    int m_gameVersion = 0;
    bool m_isSeasonal = false;

    std::vector<ServerInfo> m_allServers;
    EventSettings m_eventSettings;
    std::vector<DatabaseConfig> m_databaseConfigs;
};