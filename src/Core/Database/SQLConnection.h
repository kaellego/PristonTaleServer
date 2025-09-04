#pragma once

#include <string>
#include <vector>
#include <list> // Usaremos std::list para garantir que os ponteiros para os elementos não sejam invalidados
#include <stdexcept>
#include <optional>
#include <mutex>
#include <windows.h>
#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>

class SQLConnection {
public:
    explicit SQLConnection(const std::string& connectionString);
    ~SQLConnection();

    SQLConnection(const SQLConnection&) = delete;
    SQLConnection& operator=(const SQLConnection&) = delete;

    void prepare(const std::string& query);

    // A assinatura não muda, mas a implementação interna sim
    template<typename T>
    void bindParameter(SQLUSMALLINT index, const T& value);

    void execute();
    bool fetch();

    template<typename T>
    std::optional<T> getData(SQLUSMALLINT index);

private:
    void handleError(unsigned int handleType, SQLHANDLE handle, const std::string& context);
    void clearStatement();

    SQLHANDLE m_env = SQL_NULL_HANDLE;
    SQLHANDLE m_connection = SQL_NULL_HANDLE;
    SQLHANDLE m_statement = SQL_NULL_HANDLE;
    std::recursive_mutex m_mutex;

    // --- A CORREÇÃO PRINCIPAL ---
    // Buffer para manter os parâmetros de string vivos durante a consulta
    std::list<std::string> m_stringParamBuffer;
};

#include "Database/SQLConnection.tpp"