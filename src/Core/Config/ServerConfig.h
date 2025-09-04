#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include "Utils/INIReader.h"

// Forward declaration para a classe de leitura de INI
class INIReader;

// --- Estruturas de Dados para Configuração ---

struct ServerInfo {
    std::string name;
    std::string publicIp;
    std::string networkIp;
    uint16_t    port;
    int         maxUsers;
    bool        isLoginServer = false;
    bool        isGameServer = false;
};

struct EventSettings {
    // Adicione aqui os campos para as configurações de evento
    int  rateExp = 1;
    bool siegeWarEnabled = false;
};

// Estrutura para a configuração global do banco de dados
struct GlobalDatabaseConfig {
    std::string driver;
    std::string host;
    std::string user;
    std::string password;
    std::string netFolder;
};

class ServerConfig {
public:
    explicit ServerConfig(const std::string& filename);

    // --- Acessores Públicos (Getters) ---
    int getGameVersion() const { return m_gameVersion; }
    int getServerCode() const { return m_serverCode; }
    uint8_t getXorKey() const { return m_xorKey; }
    const GlobalDatabaseConfig& getDatabaseConfig() const;
    const ServerInfo& getThisServerInfo() const;

private:
    void loadAndValidate(INIReader& reader);
    void generateDefaultConfig(const std::string& filename) const;
    void loadServerList(INIReader& reader);
    void calculateKeys();

    // --- Membros Privados ---
    int m_gameVersion = 0;
    int m_serverCode = 0;
    uint8_t m_xorKey = 0; // O membro que armazena a chave
    std::vector<ServerInfo> m_allServers;
    GlobalDatabaseConfig m_databaseConfig;
};