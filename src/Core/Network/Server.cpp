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
Server::Server(std::shared_ptr<boost::asio::io_context> io_context, uint16_t port, PacketDispatcher& dispatcher)
// Lista de inicializa��o: a forma correta de inicializar membros em C++
    : m_io_context(io_context),
    m_acceptor(*m_io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    m_packet_dispatcher(dispatcher)
{
    std::cout << "[Rede] Servidor escutando na porta " << port << "..." << std::endl;

    // Inicia o processo de escuta por clientes.
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
    // A fun��o async_accept precisa de um socket para colocar a nova conex�o.
    // Criamos um socket que ser� preenchido quando a conex�o for estabelecida.
    m_acceptor.async_accept(
        [this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket) {
            // Este lambda � o "completion handler". Ele ser� executado pela thread
            // do io_context quando a opera��o de accept for conclu�da.

            // Verifica se a opera��o de accept foi bem-sucedida.
            if (!ec) {
                // Sucesso! O objeto 'socket' agora representa uma conex�o v�lida com um cliente.

                // Criamos uma nova sess�o para gerenciar esta conex�o.
                // Usamos std::make_shared para criar um shared_ptr que gerenciar� a vida �til da sess�o.
                // Usamos std::move(socket) para transferir a posse do socket para a sess�o de forma eficiente.
                std::make_shared<ClientSession>(std::move(socket), m_packet_dispatcher)->start();

            }
            else {
                // Ocorreu um erro ao aceitar a conex�o.
                std::cerr << "[Rede] Erro ao aceitar conexao: " << ec.message() << std::endl;
            }

            // A parte mais importante: continua o ciclo.
            // Independentemente de ter ocorrido um erro ou n�o, n�s imediatamente
            // postamos uma nova opera��o de accept para esperar pelo PR�XIMO cliente.
            do_accept();
        }
    );
}