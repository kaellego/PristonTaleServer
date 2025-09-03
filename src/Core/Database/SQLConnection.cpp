#include "Database/SQLConnection.h"
#include <vector>
#include <iostream>

SQLConnection::SQLConnection(const std::string& connectionString) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 1. Alocar Environment Handle
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_env))) {
        throw std::runtime_error("SQLConnection: Nao foi possivel alocar o handle do ambiente ODBC.");
    }

    // 2. Definir Versão do ODBC
    if (!SQL_SUCCEEDED(SQLSetEnvAttr(m_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0))) {
        // Libera o que já foi alocado antes de lançar a exceção
        SQLFreeHandle(SQL_HANDLE_ENV, m_env);
        m_env = SQL_NULL_HANDLE;
        throw std::runtime_error("SQLConnection: Nao foi possivel definir a versao do ODBC.");
    }

    // 3. Alocar Connection Handle
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DBC, m_env, &m_connection))) {
        handleError(SQL_HANDLE_ENV, m_env, "Nao foi possivel alocar o handle de conexao ODBC.");
    }

    // 4. Conectar ao Banco de Dados
    SQLCHAR connOut[1024];
    SQLSMALLINT connOutLen;
    std::vector<SQLCHAR> connStr(connectionString.begin(), connectionString.end());
    connStr.push_back('\0');

    SQLRETURN ret = SQLDriverConnect(m_connection, NULL, connStr.data(), SQL_NTS, connOut, sizeof(connOut), &connOutLen, SQL_DRIVER_NOPROMPT);
    if (!SQL_SUCCEEDED(ret)) {
        handleError(SQL_HANDLE_DBC, m_connection, "Nao foi possivel conectar ao banco de dados.");
    }

    // 5. Alocar Statement Handle
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, m_connection, &m_statement))) {
        handleError(SQL_HANDLE_DBC, m_connection, "Nao foi possivel alocar o handle de instrucao ODBC.");
    }
}

SQLConnection::~SQLConnection() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // Libera os handles na ordem inversa da alocação
    if (m_statement != SQL_NULL_HANDLE) {
        SQLFreeHandle(SQL_HANDLE_STMT, m_statement);
    }
    if (m_connection != SQL_NULL_HANDLE) {
        SQLDisconnect(m_connection);
        SQLFreeHandle(SQL_HANDLE_DBC, m_connection);
    }
    if (m_env != SQL_NULL_HANDLE) {
        SQLFreeHandle(SQL_HANDLE_ENV, m_env);
    }
}

void SQLConnection::prepare(const std::string& query) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    clearStatement();

    std::vector<SQLCHAR> queryVec(query.begin(), query.end());
    queryVec.push_back('\0');

    if (!SQL_SUCCEEDED(SQLPrepare(m_statement, queryVec.data(), SQL_NTS))) {
        handleError(SQL_HANDLE_STMT, m_statement, "Falha ao preparar a query: " + query);
    }
}

void SQLConnection::execute() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    SQLRETURN ret = SQLExecute(m_statement);
    if (!SQL_SUCCEEDED(ret)) {
        handleError(SQL_HANDLE_STMT, m_statement, "Falha ao executar a instrucao.");
    }
}

bool SQLConnection::fetch() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    SQLRETURN ret = SQLFetch(m_statement);
    if (ret == SQL_NO_DATA) {
        return false;
    }
    if (!SQL_SUCCEEDED(ret)) {
        handleError(SQL_HANDLE_STMT, m_statement, "Falha ao buscar a proxima linha (fetch).");
    }
    return true;
}

void SQLConnection::clearStatement() {
    // Não precisa de lock aqui, pois é chamada por métodos públicos que já têm lock
    SQLFreeStmt(m_statement, SQL_CLOSE);
    SQLFreeStmt(m_statement, SQL_UNBIND);
    SQLFreeStmt(m_statement, SQL_RESET_PARAMS);
}

void SQLConnection::handleError(unsigned int handleType, SQLHANDLE handle, const std::string& context) {
    SQLCHAR sqlstate[6];
    SQLINTEGER nativeError;
    SQLCHAR messageText[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT textLength;
    std::string errorMsg = "[ODBC Error] " + context + "\n";

    SQLGetDiagRec(handleType, handle, 1, sqlstate, &nativeError, messageText, sizeof(messageText), &textLength);

    errorMsg += "  SQLSTATE: " + std::string(reinterpret_cast<const char*>(sqlstate)) + "\n";
    errorMsg += "  Message: " + std::string(reinterpret_cast<const char*>(messageText));

    // A exceção fará com que o destrutor seja chamado (se o objeto foi totalmente construído),
    // garantindo a limpeza dos recursos.
    throw std::runtime_error(errorMsg);
}