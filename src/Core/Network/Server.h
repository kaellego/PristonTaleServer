#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <cstdint>

// Forward declarations para desacoplar cabeçalhos
class PacketDispatcher;
class ClientSession;

/**
 * @class Server
 * @brief Aceita conexões TCP de entrada de forma assíncrona.
 *
 * Esta classe é responsável por escutar em uma porta de rede específica.
 * Para cada nova conexão bem-sucedida, ela instancia um ClientSession
 * para gerenciar a comunicação com aquele cliente.
 */
class Server {
public:
    /**
     * @brief Construtor do servidor de rede.
     * @param io_context O contexto de I/O compartilhado para operações assíncronas.
     * @param port A porta em que o servidor escutará por conexões.
     * @param dispatcher A referência para o dispatcher de pacotes que será passado para novas sessões.
     */
    explicit Server(std::shared_ptr<boost::asio::io_context> io_context, uint16_t port, PacketDispatcher& dispatcher);

private:
    /**
     * @brief Inicia o ciclo assíncrono de aceitação de uma nova conexão.
     */
    void do_accept();

    // O contexto de I/O do Boost.Asio, compartilhado por toda a aplicação.
    std::shared_ptr<boost::asio::io_context> m_io_context;

    // O objeto do Asio que escuta por novas conexões TCP.
    boost::asio::ip::tcp::acceptor m_acceptor;

    // Referência para o dispatcher que será injetado em cada nova sessão de cliente.
    PacketDispatcher& m_packet_dispatcher;
};