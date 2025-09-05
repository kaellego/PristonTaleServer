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
class DatabasePool; // Necessário para a declaração de PooledConnection

enum class EDatabaseID { GameDB, LogDB, UserDB_Primary, Invalid };

/**
 * @class PooledConnection
 * @brief Um invólucro RAII para uma SQLConnection emprestada do pool.
 * Garante que a conexão seja devolvida automaticamente ao pool quando sair de escopo.
 */
class PooledConnection {
public:
    // Permite usar o objeto como um ponteiro (ex: conn->prepare(...))
    SQLConnection* operator->() const {
        return m_connection;
    }

    // Permite obter o ponteiro bruto, se necessário
    SQLConnection* get() const {
        return m_connection;
    }

    /**
     * @brief Permite que o objeto seja usado em contextos booleanos (ex: if (conn)).
     * @return true se a conexão for válida (não nula), false caso contrário.
     */
    explicit operator bool() const {
        return m_connection != nullptr;
    }

    // Impede a cópia para garantir que a conexão só seja devolvida uma vez.
    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;

    // Construtor de movimentação para permitir que a função getConnection a retorne.
    PooledConnection(PooledConnection&& other) noexcept;
    PooledConnection& operator=(PooledConnection&& other) noexcept;

    // Destrutor que devolve a conexão ao pool.
    ~PooledConnection();

private:
    // Só o DatabasePool pode criar instâncias desta classe.
    friend class DatabasePool;
    PooledConnection(SQLConnection* connection, DatabasePool& pool);

    SQLConnection* m_connection;
    DatabasePool* m_pool;
};

/**
 * @class DatabasePool
 * @brief Gerencia um pool de conexões de banco de dados thread-safe.
 */
class DatabasePool {
public:
    explicit DatabasePool(const ServerConfig& config, LogService& logService, size_t poolSizePerDb = 5);
    ~DatabasePool();

    DatabasePool(const DatabasePool&) = delete;
    DatabasePool& operator=(const DatabasePool&) = delete;

    /**
     * @brief Pega emprestada uma conexão do pool para um banco de dados específico.
     * Se o pool estiver vazio, a chamada irá bloquear até uma conexão ser devolvida.
     * @param id O identificador do banco de dados.
     * @return Um objeto RAII PooledConnection que gerencia a conexão.
     */
    PooledConnection getConnection(EDatabaseID id);

private:
    friend class PooledConnection;
    /**
     * @brief Devolve uma conexão ao pool. Chamado pelo destrutor de PooledConnection.
     * @param connection O ponteiro para a SQLConnection a ser devolvida.
     */
    void returnConnection(SQLConnection* connection);

    LogService& m_logService;

    // Um mapa que armazena um pool (uma pilha) de conexões para cada ID de banco de dados.
    std::map<EDatabaseID, std::stack<std::unique_ptr<SQLConnection>>> m_pools;

    // Mapa para associar uma conexão devolvida ao seu pool correto.
    std::map<SQLConnection*, EDatabaseID> m_connectionToPoolMap;

    std::mutex m_mutex;
    std::condition_variable m_condition;
};