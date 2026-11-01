#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <vector>

// Forward declarations para desacoplar cabe?alhos
class PacketDispatcher;
class ClientSession;
class LogService;

/**
 * @class Server
 * @brief Aceita conex?es TCP de entrada de forma ass?ncrona.
 *
 * Esta classe ? respons?vel por escutar em uma porta de rede espec?fica.
 * Para cada nova conex?o bem-sucedida, ela instancia um ClientSession
 * para gerenciar a comunica??o com aquele cliente.
 */
class Server {
public:
    /**
     * @brief Construtor do servidor de rede.
     * @param io_context O contexto de I/O compartilhado para opera??es ass?ncronas.
     * @param port A porta em que o servidor escutar? por conex?es.
     * @param dispatcher A refer?ncia para o dispatcher de pacotes que ser? passado para novas sess?es.
     */
    explicit Server(std::shared_ptr<boost::asio::io_context> io_context, uint16_t port, PacketDispatcher& dispatcher, LogService& logService, uint8_t xorKey, bool gameEndpoint = false);
    void stop();

private:
    /**
     * @brief Inicia o ciclo ass?ncrono de aceita??o de uma nova conex?o.
     */
    void do_accept();

    // O contexto de I/O do Boost.Asio, compartilhado por toda a aplica??o.
    std::shared_ptr<boost::asio::io_context> m_io_context;

    // O objeto do Asio que escuta por novas conex?es TCP.
    boost::asio::ip::tcp::acceptor m_acceptor;

    // Refer?ncia para o dispatcher que ser? injetado em cada nova sess?o de cliente.
    PacketDispatcher& m_packet_dispatcher;
    LogService& m_logService;
    uint8_t m_xorKey;
    bool m_gameEndpoint;
    std::atomic_bool m_stopped{false};
    std::mutex m_sessionsMutex;
    std::vector<std::weak_ptr<ClientSession>> m_sessions;
};
