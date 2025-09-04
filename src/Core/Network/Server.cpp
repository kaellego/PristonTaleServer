#include "Network/Server.h"

// Incluímos as definições completas aqui, pois são necessárias para a implementação.
#include "Network/ClientSession.h"
#include "Network/PacketDispatcher.h"

#include <iostream>
#include <utility> // Para std::move

/**
 * @brief Construtor do Server.
 *
 * Inicializa os membros e, mais importante, inicia o ciclo de aceitação de conexões
 * chamando do_accept() pela primeira vez.
 */
Server::Server(std::shared_ptr<boost::asio::io_context> io_context, uint16_t port, PacketDispatcher& dispatcher, LogService& logService, uint8_t xorKey)
    : m_io_context(io_context),
    m_acceptor(*io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    m_packet_dispatcher(dispatcher),
    m_logService(logService),
    m_xorKey(xorKey) // <-- Armazena a chave
{
    std::cout << "[Rede] Servidor escutando na porta " << port << "..." << std::endl;
    do_accept();
}

/**
 * @brief O coração do servidor: o "accept loop" assíncrono.
 *
 * Esta função posta uma operação de "accept" no io_context. A função retorna
 * imediatamente. O io_context chamará o lambda de callback quando uma nova
 * conexão chegar.
 */
void Server::do_accept() {
    m_acceptor.async_accept(
        [this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                // CORRIGIDO: Passa a chave XOR para o construtor do ClientSession
                std::make_shared<ClientSession>(std::move(socket), m_packet_dispatcher, m_logService, m_xorKey)->start();
            }
            else {
                std::cerr << "[Rede] Erro ao aceitar conexao: " << ec.message() << std::endl;
            }
            do_accept();
        }
    );
}