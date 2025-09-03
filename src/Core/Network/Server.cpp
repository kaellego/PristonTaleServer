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
Server::Server(std::shared_ptr<boost::asio::io_context> io_context, uint16_t port, PacketDispatcher& dispatcher)
// Lista de inicialização: a forma correta de inicializar membros em C++
    : m_io_context(io_context),
    m_acceptor(*m_io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    m_packet_dispatcher(dispatcher)
{
    std::cout << "[Rede] Servidor escutando na porta " << port << "..." << std::endl;

    // Inicia o processo de escuta por clientes.
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
    // A função async_accept precisa de um socket para colocar a nova conexão.
    // Criamos um socket que será preenchido quando a conexão for estabelecida.
    m_acceptor.async_accept(
        [this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket) {
            // Este lambda é o "completion handler". Ele será executado pela thread
            // do io_context quando a operação de accept for concluída.

            // Verifica se a operação de accept foi bem-sucedida.
            if (!ec) {
                // Sucesso! O objeto 'socket' agora representa uma conexão válida com um cliente.

                // Criamos uma nova sessão para gerenciar esta conexão.
                // Usamos std::make_shared para criar um shared_ptr que gerenciará a vida útil da sessão.
                // Usamos std::move(socket) para transferir a posse do socket para a sessão de forma eficiente.
                std::make_shared<ClientSession>(std::move(socket), m_packet_dispatcher)->start();

            }
            else {
                // Ocorreu um erro ao aceitar a conexão.
                std::cerr << "[Rede] Erro ao aceitar conexao: " << ec.message() << std::endl;
            }

            // A parte mais importante: continua o ciclo.
            // Independentemente de ter ocorrido um erro ou não, nós imediatamente
            // postamos uma nova operação de accept para esperar pelo PRÓXIMO cliente.
            do_accept();
        }
    );
}