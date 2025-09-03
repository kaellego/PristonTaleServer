#pragma once

#include <map>
#include <memory>
#include <string>
#include "Config/ServerConfig.h" // Precisa da definição de DatabaseConfig

// Forward declaration para evitar a inclusão do header completo aqui,
// o que melhora os tempos de compilação e o desacoplamento.
class SQLConnection;

// Enum para identificar as conexões de forma mais segura e legível.
// Este enum deve ser movido para um arquivo comum se usado em outros lugares.
enum class EDatabaseID {
    GameDB,
    LogDB,
    UserDB_Primary,
    // Adicione outros IDs conforme necessário...
    Invalid
};

/**
 * @class DatabaseManager
 * @brief Gerencia o ciclo de vida e o acesso a todas as conexões de banco de dados.
 *
 * Esta classe é responsável por inicializar as conexões SQL com base na configuração
 * do servidor e fornecer ponteiros para essas conexões de forma segura.
 * Ela possui as conexões, garantindo a limpeza automática dos recursos.
 */
class DatabaseManager {
public:
    /**
     * @brief Constrói e inicializa o gerenciador de banco de dados.
     * @param config O objeto de configuração do servidor contendo os detalhes da conexão.
     */
    explicit DatabaseManager(const ServerConfig& config);

    /**
     * @brief Destrutor. Limpa automaticamente todas as conexões.
     */
    ~DatabaseManager();

    /**
     * @brief Obtém um ponteiro para uma conexão de banco de dados específica.
     * @param id O identificador da conexão desejada (ex: EDatabaseID::GameDB).
     * @return Um ponteiro bruto para o objeto SQLConnection, ou nullptr se não for encontrado.
     */
    SQLConnection* getConnection(EDatabaseID id);

private:
    // Mapeia o nome string da config para o nosso enum de ID
    EDatabaseID mapStringToEnum(const std::string& name) const;

    // Um mapa para armazenar as conexões. A chave é o ID e o valor é um
    // ponteiro inteligente que gerencia a vida útil do objeto SQLConnection.
    std::map<EDatabaseID, std::unique_ptr<SQLConnection>> m_connections;
};