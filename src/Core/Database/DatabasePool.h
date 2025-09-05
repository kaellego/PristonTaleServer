#pragma once

#include <map>
#include <memory>
#include <string>
#include <stack>
#include <mutex>
#include <condition_variable>
#include "Shared/Constants.h" // Para EDatabaseID

// Forward declarations
class ServerConfig;
class LogService;
class SQLConnection;
class DatabasePool; // Necess�rio para a declara��o de PooledConnection

enum class EDatabaseID { GameDB, LogDB, UserDB_Primary, Invalid };

/**
 * @class PooledConnection
 * @brief Um inv�lucro RAII para uma SQLConnection emprestada do pool.
 * Garante que a conex�o seja devolvida automaticamente ao pool quando sair de escopo.
 */
class PooledConnection {
public:
    // Permite usar o objeto como um ponteiro (ex: conn->prepare(...))
    SQLConnection* operator->() const {
        return m_connection;
    }

    // Permite obter o ponteiro bruto, se necess�rio
    SQLConnection* get() const {
        return m_connection;
    }

    /**
     * @brief Permite que o objeto seja usado em contextos booleanos (ex: if (conn)).
     * @return true se a conex�o for v�lida (n�o nula), false caso contr�rio.
     */
    explicit operator bool() const {
        return m_connection != nullptr;
    }

    // Impede a c�pia para garantir que a conex�o s� seja devolvida uma vez.
    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;

    // Construtor de movimenta��o para permitir que a fun��o getConnection a retorne.
    PooledConnection(PooledConnection&& other) noexcept;
    PooledConnection& operator=(PooledConnection&& other) noexcept;

    // Destrutor que devolve a conex�o ao pool.
    ~PooledConnection();

private:
    // S� o DatabasePool pode criar inst�ncias desta classe.
    friend class DatabasePool;
    PooledConnection(SQLConnection* connection, DatabasePool& pool);

    SQLConnection* m_connection;
    DatabasePool* m_pool;
};

/**
 * @class DatabasePool
 * @brief Gerencia um pool de conex�es de banco de dados thread-safe.
 */
class DatabasePool {
public:
    explicit DatabasePool(const ServerConfig& config, LogService& logService, size_t poolSizePerDb = 5);
    ~DatabasePool();

    DatabasePool(const DatabasePool&) = delete;
    DatabasePool& operator=(const DatabasePool&) = delete;

    /**
     * @brief Pega emprestada uma conex�o do pool para um banco de dados espec�fico.
     * Se o pool estiver vazio, a chamada ir� bloquear at� uma conex�o ser devolvida.
     * @param id O identificador do banco de dados.
     * @return Um objeto RAII PooledConnection que gerencia a conex�o.
     */
    PooledConnection getConnection(EDatabaseID id);

private:
    friend class PooledConnection;
    /**
     * @brief Devolve uma conex�o ao pool. Chamado pelo destrutor de PooledConnection.
     * @param connection O ponteiro para a SQLConnection a ser devolvida.
     */
    void returnConnection(SQLConnection* connection);

    LogService& m_logService;

    // Um mapa que armazena um pool (uma pilha) de conex�es para cada ID de banco de dados.
    std::map<EDatabaseID, std::stack<std::unique_ptr<SQLConnection>>> m_pools;

    // Mapa para associar uma conex�o devolvida ao seu pool correto.
    std::map<SQLConnection*, EDatabaseID> m_connectionToPoolMap;

    std::mutex m_mutex;
    std::condition_variable m_condition;
};