// Este arquivo deve ser incluído no final de SQLConnection.h

#include <type_traits> // Para std::is_integral_v, etc.

template<typename T>
void SQLConnection::bindParameter(SQLUSMALLINT index, const T& value) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    SQLSMALLINT cType = 0;
    SQLSMALLINT sqlType = 0;
    SQLLEN size = 0;
    SQLPOINTER ptr = (SQLPOINTER)&value;

    if constexpr (std::is_same_v<T, bool>) {
        cType = SQL_C_BIT;
        sqlType = SQL_BIT;
    } else if constexpr (std::is_integral_v<T> && sizeof(T) == 1) { // char, uint8_t
        cType = SQL_C_TINYINT;
        sqlType = SQL_TINYINT;
    } else if constexpr (std::is_integral_v<T> && sizeof(T) == 2) { // short
        cType = SQL_C_SHORT;
        sqlType = SQL_SMALLINT;
    } else if constexpr (std::is_integral_v<T> && sizeof(T) == 4) { // int
        cType = SQL_C_LONG;
        sqlType = SQL_INTEGER;
    } else if constexpr (std::is_integral_v<T> && sizeof(T) == 8) { // int64_t
        cType = SQL_C_SBIGINT;
        sqlType = SQL_BIGINT;
    } else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4) { // float
        cType = SQL_C_FLOAT;
        sqlType = SQL_REAL;
    } else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8) { // double
        cType = SQL_C_DOUBLE;
        sqlType = SQL_DOUBLE;
    } else if constexpr (std::is_same_v<T, std::string>) {
        cType = SQL_C_CHAR;
        sqlType = SQL_VARCHAR;
        ptr = (SQLPOINTER)const_cast<char*>(value.c_str());
        size = value.length();
    } else {
        // static_assert irá falhar a compilação se um tipo não suportado for usado.
        static_assert(sizeof(T) == -1, "Tipo de dado nao suportado para SQLConnection::bindParameter");
    }

    SQLRETURN ret = SQLBindParameter(m_statement, index, SQL_PARAM_INPUT, cType, sqlType, size, 0, ptr, 0, (SQLLEN*)SQL_NTS);   
    if (!SQL_SUCCEEDED(ret)) {
        handleError(SQL_HANDLE_STMT, m_statement, "Falha ao associar (bind) parametro " + std::to_string(index));
    }
}

template<typename T>
std::optional<T> SQLConnection::getData(SQLUSMALLINT index) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    T result{};
    SQLLEN indicator;
    SQLSMALLINT cType = 0;
    SQLLEN bufferLength = 0;
    
    if constexpr (std::is_same_v<T, bool>) { cType = SQL_C_BIT; }
    else if constexpr (std::is_integral_v<T> && sizeof(T) == 1) { cType = SQL_C_TINYINT; }
    else if constexpr (std::is_integral_v<T> && sizeof(T) == 2) { cType = SQL_C_SHORT; }
    else if constexpr (std::is_integral_v<T> && sizeof(T) == 4) { cType = SQL_C_LONG; }
    else if constexpr (std::is_integral_v<T> && sizeof(T) == 8) { cType = SQL_C_SBIGINT; }
    else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4) { cType = SQL_C_FLOAT; }
    else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8) { cType = SQL_C_DOUBLE; }
    else if constexpr (std::is_same_v<T, std::string>) { cType = SQL_C_CHAR; bufferLength = 2048; /* Tamanho maximo do buffer para strings */ }
    else { static_assert(sizeof(T) == -1, "Tipo de dado nao suportado para SQLConnection::getData"); }

    std::vector<char> buffer(bufferLength > 0 ? bufferLength : 0);
    SQLPOINTER ptr = bufferLength > 0 ? (SQLPOINTER)buffer.data() : (SQLPOINTER)&result;

    SQLRETURN ret = SQLGetData(m_statement, index, cType, ptr, bufferLength, &indicator);

    if (ret == SQL_NO_DATA) {
        return std::nullopt; // Não há mais linhas
    } else if (!SQL_SUCCEEDED(ret)) {
        handleError(SQL_HANDLE_STMT, m_statement, "Falha ao obter dados (getData) da coluna " + std::to_string(index));
    }

    if (indicator == SQL_NULL_DATA) {
        return std::nullopt; // O valor na coluna é NULL
    }
    
    if constexpr (std::is_same_v<T, std::string>) {
        return std::string(buffer.data());
    } else {
        return result;
    }
}