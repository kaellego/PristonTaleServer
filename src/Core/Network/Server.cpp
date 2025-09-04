#include "Network/Server.h"

// Inclu�mos as defini��es completas aqui, pois s�o necess�rias para a implementa��o.
#include "Network/ClientSession.h"
#include "Network/PacketDispatcher.h"

#include <iostream>
#include <utility> // Para std::move

/**
 * @brief Construtor do Server.
 *
 * Inicializa os membros e, mais importante, inicia o ciclo de aceita��o de conex�es
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
 * @brief O cora��o do servidor: o "accept loop" ass�ncrono.
 *
 * Esta fun��o posta uma opera��o de "accept" no io_context. A fun��o retorna
 * imediatamente. O io_context chamar� o lambda de callback quando uma nova
 * conex�o chegar.
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