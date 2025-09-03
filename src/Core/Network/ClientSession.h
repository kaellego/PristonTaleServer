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
 * @brief Gerencia uma �nica conex�o de cliente de forma ass�ncrona.
 *
 * Esta classe � respons�vel por todo o ciclo de I/O de um cliente,
 * incluindo a leitura de pacotes, o despacho para processamento
 * e o envio de respostas. Sua vida �til � gerenciada por std::shared_ptr.
 */
class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    /**
     * @brief Construtor que assume a posse de um socket rec�m-aceito.
     * @param socket O socket TCP para este cliente.
     * @param dispatcher O objeto que processar� os pacotes recebidos.
     */
    ClientSession(boost::asio::ip::tcp::socket socket, PacketDispatcher& dispatcher);

    /**
     * @brief Destrutor. Apenas para log.
     */
    ~ClientSession();

    /**
     * @brief Inicia o ciclo de opera��es ass�ncronas para esta sess�o.
     * Deve ser chamado uma vez ap�s a cria��o da sess�o.
     */
    void start();

    /**
     * @brief Enfileira um pacote de dados para envio ao cliente.
     * A opera��o de escrita � ass�ncrona.
     * @param packet O pacote a ser enviado.
     */
    void send(const Packet& packet);

    /**
     * @brief Retorna o socket associado a esta sess�o.
     */
    boost::asio::ip::tcp::socket& socket();

private:
    /**
     * @brief Inicia uma opera��o de leitura ass�ncrona para o cabe�alho do pacote.
     */
    void do_read_header();

    /**
     * @brief Inicia uma opera��o de leitura ass�ncrona para o corpo do pacote.
     * @param body_length O tamanho do corpo a ser lido, determinado pelo cabe�alho.
     */
    void do_read_body(uint16_t body_length);

    /**
     * @brief Inicia uma opera��o de escrita ass�ncrona para o primeiro pacote na fila.
     */
    void do_write();

    /**
     * @brief Encerra a conex�o e o socket de forma graciosa.
     */
    void close();

    boost::asio::ip::tcp::socket m_socket;
    PacketDispatcher& m_packet_dispatcher;

    // Buffer de leitura para o cabe�alho
    PacketHeader m_read_header;
    // Buffer de leitura para o corpo do pacote
    std::vector<uint8_t> m_read_body;

    // Fila de pacotes para envio. Usamos um deque para efici�ncia na inser��o/remo��o.
    std::deque<Packet> m_write_queue;
};