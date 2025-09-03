#pragma once

#include <string>
#include <stdexcept>
#include <optional>
#include <mutex>

// Inclui os cabeçalhos da API ODBC do Windows
#include <windows.h>
#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>

/**
 * @class SQLConnection
 * @brief Gerencia uma única conexão com um banco de dados via ODBC de forma segura (RAII).
 *
 * A conexão é estabelecida no construtor e liberada no destrutor. A classe é
 * thread-safe para operações em um mesmo objeto e usa templates para type-safety.
 */
class SQLConnection {
public:
    /**
     * @brief Constrói o objeto e se conecta ao banco de dados.
     * @param connectionString A string de conexão ODBC completa.
     * @throws std::runtime_error Se a conexão falhar.
     */
    explicit SQLConnection(const std::string& connectionString);

    /**
     * @brief Destrutor. Garante a desconexão e a liberação de todos os handles ODBC.
     */
    ~SQLConnection();

    // Desabilita a cópia e a movimentação para evitar duplicação de handles.
    SQLConnection(const SQLConnection&) = delete;
    SQLConnection& operator=(const SQLConnection&) = delete;
    SQLConnection(SQLConnection&&) = delete;
    SQLConnection& operator=(SQLConnection&&) = delete;

    /**
     * @brief Prepara uma instrução SQL para execução.
     * @param query A consulta SQL com placeholders (?).
     */
    void prepare(const std::string& query);

    /**
     * @brief Associa um valor a um placeholder na consulta preparada.
     * @tparam T O tipo do valor a ser associado.
     * @param index O índice do placeholder (começando em 1).
     * @param value O valor a ser associado.
     */
    template<typename T>
    void bindParameter(SQLUSMALLINT index, const T& value);

    /**
     * @brief Executa a instrução preparada.
     */
    void execute();

    /**
     * @brief Avança para a próxima linha no conjunto de resultados.
     * @return true se uma linha foi encontrada, false se não houver mais linhas.
     */
    bool fetch();

    /**
     * @brief Obtém o valor de uma coluna da linha atual.
     * @tparam T O tipo de dado esperado para a coluna.
     * @param index O índice da coluna (começando em 1).
     * @return std::optional<T> contendo o valor, ou vazio se o valor for NULL no banco de dados.
     */
    template<typename T>
    std::optional<T> getData(SQLUSMALLINT index);

private:
    void handleError(unsigned int handleType, SQLHANDLE handle, const std::string& context);
    void clearStatement();

    SQLHANDLE m_env = SQL_NULL_HANDLE;
    SQLHANDLE m_connection = SQL_NULL_HANDLE;
    SQLHANDLE m_statement = SQL_NULL_HANDLE;
    std::recursive_mutex m_mutex; // Mutex para garantir thread-safety
};

// Inclui o arquivo de implementação dos templates
#include "Database/SQLConnection.tpp"