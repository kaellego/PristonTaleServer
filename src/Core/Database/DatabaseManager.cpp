#include "Database/DatabaseManager.h"
#include "Database/SQLConnection.h"
#include <stdexcept>
#include <iostream>
#include <string> // Para std::to_string

// O construtor itera sobre as configurações de DB fornecidas pelo ServerConfig
// e cria uma instância de SQLConnection para cada uma.
DatabaseManager::DatabaseManager(const ServerConfig& config) {
    std::cout << "Inicializando gerenciador de banco de dados..." << std::endl;

    for (const auto& dbConfig : config.getDatabaseConfigs()) {
        // Mapeia o nome da string (ex: "GameDB") para o enum correspondente.
        EDatabaseID id = mapStringToEnum(dbConfig.name);
        if (id == EDatabaseID::Invalid) {
            std::cerr << "AVISO: Nome de conexao de banco de dados desconhecido no server.ini: " << dbConfig.name << std::endl;
            continue;
        }

        try {
            // Monta a string de conexão a partir da configuração.
            // Adapte o "Driver" para o que você usa (ex: "ODBC Driver 17 for SQL Server")
            std::string connStr = "Driver={SQL Server};Server=" + dbConfig.host +
                "," + std::to_string(dbConfig.port) +
                ";Database=" + dbConfig.database +
                ";Uid=" + dbConfig.user +
                ";Pwd=" + dbConfig.password + ";";

            // Cria a conexão. Se falhar, o construtor da SQLConnection lança uma exceção.
            // O objeto só é criado se a conexão for bem-sucedida.
            auto connection = std::make_unique<SQLConnection>(connStr);

            std::cout << "  - Conexao '" << dbConfig.name << "' estabelecida com sucesso." << std::endl;

            // Move a posse do ponteiro para o mapa.
            // As chamadas para Open() e Close() foram removidas pois são automáticas.
            m_connections[id] = std::move(connection);

        }
        catch (const std::runtime_error& e) {
            // Propaga o erro fatal para a classe Application
            throw std::runtime_error("Falha ao criar conexao para '" + dbConfig.name + "': " + e.what());
        }
    }
    std::cout << "Gerenciador de banco de dados inicializado." << std::endl;
}

DatabaseManager::~DatabaseManager() {
    std::cout << "Encerrando gerenciador de banco de dados e fechando conexoes..." << std::endl;
    m_connections.clear();
    std::cout << "Conexoes de banco de dados encerradas." << std::endl;
}

SQLConnection* DatabaseManager::getConnection(EDatabaseID id) {
    auto it = m_connections.find(id);
    if (it != m_connections.end()) {
        return it->second.get();
    }
    return nullptr;
}

EDatabaseID DatabaseManager::mapStringToEnum(const std::string& name) const {
    // Este mapeamento deve ser preenchido com todos os seus IDs
    if (name == "GameDB")         return EDatabaseID::GameDB;
    if (name == "LogDB")          return EDatabaseID::LogDB;
    if (name == "UserDB_Primary") return EDatabaseID::UserDB_Primary;
    // ... adicione outros mapeamentos aqui
    return EDatabaseID::Invalid;
}