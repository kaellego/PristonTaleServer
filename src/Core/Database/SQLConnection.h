#pragma once

#include <string>
#include <stdexcept>
#include <optional>
#include <mutex>

// Inclui os cabe�alhos da API ODBC do Windows
#include <windows.h>
#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>

/**
 * @class SQLConnection
 * @brief Gerencia uma �nica conex�o com um banco de dados via ODBC de forma segura (RAII).
 *
 * A conex�o � estabelecida no construtor e liberada no destrutor. A classe �
 * thread-safe para opera��es em um mesmo objeto e usa templates para type-safety.
 */
class SQLConnection {
public:
    /**
     * @brief Constr�i o objeto e se conecta ao banco de dados.
     * @param connectionString A string de conex�o ODBC completa.
     * @throws std::runtime_error Se a conex�o falhar.
     */
    explicit SQLConnection(const std::string& connectionString);

    /**
     * @brief Destrutor. Garante a desconex�o e a libera��o de todos os handles ODBC.
     */
    ~SQLConnection();

    // Desabilita a c�pia e a movimenta��o para evitar duplica��o de handles.
    SQLConnection(const SQLConnection&) = delete;
    SQLConnection& operator=(const SQLConnection&) = delete;
    SQLConnection(SQLConnection&&) = delete;
    SQLConnection& operator=(SQLConnection&&) = delete;

    /**
     * @brief Prepara uma instru��o SQL para execu��o.
     * @param query A consulta SQL com placeholders (?).
     */
    void prepare(const std::string& query);

    /**
     * @brief Associa um valor a um placeholder na consulta preparada.
     * @tparam T O tipo do valor a ser associado.
     * @param index O �ndice do placeholder (come�ando em 1).
     * @param value O valor a ser associado.
     */
    template<typename T>
    void bindParameter(SQLUSMALLINT index, const T& value);

    /**
     * @brief Executa a instru��o preparada.
     */
    void execute();

    /**
     * @brief Avan�a para a pr�xima linha no conjunto de resultados.
     * @return true se uma linha foi encontrada, false se n�o houver mais linhas.
     */
    bool fetch();

    /**
     * @brief Obt�m o valor de uma coluna da linha atual.
     * @tparam T O tipo de dado esperado para a coluna.
     * @param index O �ndice da coluna (come�ando em 1).
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

// Inclui o arquivo de implementa��o dos templates
#include "Database/SQLConnection.tpp"