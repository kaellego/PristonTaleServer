#pragma once

#include <map>
#include <memory>
#include <string>
#include "Config/ServerConfig.h" // Precisa da defini��o de DatabaseConfig

// Forward declaration para evitar a inclus�o do header completo aqui,
// o que melhora os tempos de compila��o e o desacoplamento.
class SQLConnection;

// Enum para identificar as conex�es de forma mais segura e leg�vel.
// Este enum deve ser movido para um arquivo comum se usado em outros lugares.
enum class EDatabaseID {
    GameDB,
    LogDB,
    UserDB_Primary,
    // Adicione outros IDs conforme necess�rio...
    Invalid
};

/**
 * @class DatabaseManager
 * @brief Gerencia o ciclo de vida e o acesso a todas as conex�es de banco de dados.
 *
 * Esta classe � respons�vel por inicializar as conex�es SQL com base na configura��o
 * do servidor e fornecer ponteiros para essas conex�es de forma segura.
 * Ela possui as conex�es, garantindo a limpeza autom�tica dos recursos.
 */
class DatabaseManager {
public:
    /**
     * @brief Constr�i e inicializa o gerenciador de banco de dados.
     * @param config O objeto de configura��o do servidor contendo os detalhes da conex�o.
     */
    explicit DatabaseManager(const ServerConfig& config);

    /**
     * @brief Destrutor. Limpa automaticamente todas as conex�es.
     */
    ~DatabaseManager();

    /**
     * @brief Obt�m um ponteiro para uma conex�o de banco de dados espec�fica.
     * @param id O identificador da conex�o desejada (ex: EDatabaseID::GameDB).
     * @return Um ponteiro bruto para o objeto SQLConnection, ou nullptr se n�o for encontrado.
     */
    SQLConnection* getConnection(EDatabaseID id);

private:
    // Mapeia o nome string da config para o nosso enum de ID
    EDatabaseID mapStringToEnum(const std::string& name) const;

    // Um mapa para armazenar as conex�es. A chave � o ID e o valor � um
    // ponteiro inteligente que gerencia a vida �til do objeto SQLConnection.
    std::map<EDatabaseID, std::unique_ptr<SQLConnection>> m_connections;
};