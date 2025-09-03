#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <deque>
#include "Network/Packet.h"

// Forward declarations
class PacketDispatcher;

/**
 * @class ClientSession
 * @brief Gerencia uma única conexão de cliente de forma assíncrona.
 *
 * Esta classe é responsável por todo o ciclo de I/O de um cliente,
 * incluindo a leitura de pacotes, o despacho para processamento
 * e o envio de respostas. Sua vida útil é gerenciada por std::shared_ptr.
 */
class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    /**
     * @brief Construtor que assume a posse de um socket recém-aceito.
     * @param socket O socket TCP para este cliente.
     * @param dispatcher O objeto que processará os pacotes recebidos.
     */
    ClientSession(boost::asio::ip::tcp::socket socket, PacketDispatcher& dispatcher);

    /**
     * @brief Destrutor. Apenas para log.
     */
    ~ClientSession();

    /**
     * @brief Inicia o ciclo de operações assíncronas para esta sessão.
     * Deve ser chamado uma vez após a criação da sessão.
     */
    void start();

    /**
     * @brief Enfileira um pacote de dados para envio ao cliente.
     * A operação de escrita é assíncrona.
     * @param packet O pacote a ser enviado.
     */
    void send(const Packet& packet);

    /**
     * @brief Retorna o socket associado a esta sessão.
     */
    boost::asio::ip::tcp::socket& socket();

private:
    /**
     * @brief Inicia uma operação de leitura assíncrona para o cabeçalho do pacote.
     */
    void do_read_header();

    /**
     * @brief Inicia uma operação de leitura assíncrona para o corpo do pacote.
     * @param body_length O tamanho do corpo a ser lido, determinado pelo cabeçalho.
     */
    void do_read_body(uint16_t body_length);

    /**
     * @brief Inicia uma operação de escrita assíncrona para o primeiro pacote na fila.
     */
    void do_write();

    /**
     * @brief Encerra a conexão e o socket de forma graciosa.
     */
    void close();

    boost::asio::ip::tcp::socket m_socket;
    PacketDispatcher& m_packet_dispatcher;

    // Buffer de leitura para o cabeçalho
    PacketHeader m_read_header;
    // Buffer de leitura para o corpo do pacote
    std::vector<uint8_t> m_read_body;

    // Fila de pacotes para envio. Usamos um deque para eficiência na inserção/remoção.
    std::deque<Packet> m_write_queue;
};