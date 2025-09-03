#include "Network/ClientSession.h"
#include "Network/PacketDispatcher.h" // A implementa��o precisa da defini��o completa
#include <iostream>

ClientSession::ClientSession(boost::asio::ip::tcp::socket socket, PacketDispatcher& dispatcher)
    : m_socket(std::move(socket)), m_packet_dispatcher(dispatcher) {}

ClientSession::~ClientSession() {
    // Log para depura��o, para sabermos quando uma sess�o � destru�da.
    try {
        std::cout << "Sessao encerrada para " << m_socket.remote_endpoint() << std::endl;
    }
    catch (const boost::system::system_error& e) {
        std::cout << "Sessao encerrada para um socket ja desconectado. Erro: " << e.what() << std::endl;
    }
}

boost::asio::ip::tcp::socket& ClientSession::socket() {
    return m_socket;
}

void ClientSession::start() {
    try {
        std::cout << "Nova sessao iniciada: " << m_socket.remote_endpoint() << std::endl;
    }
    catch (const boost::system::system_error& e) {
        std::cerr << "ERRO: Nao foi possivel obter o endpoint remoto ao iniciar a sessao: " << e.what() << std::endl;
        return; // N�o podemos continuar se o socket j� estiver fechado.
    }

    // Inicia o ciclo de leitura ass�ncrona.
    do_read_header();
}

void ClientSession::do_read_header() {
    // Usamos shared_from_this() para garantir que o objeto ClientSession
    // permane�a vivo enquanto a opera��o ass�ncrona estiver pendente.
    auto self = shared_from_this();
    boost::asio::async_read(m_socket,
        boost::asio::buffer(&m_read_header, sizeof(PacketHeader)),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                // TODO: Descriptografar o cabe�alho se necess�rio.

                // Valida o tamanho do pacote para evitar ataques de buffer overflow.
                if (m_read_header.length >= sizeof(PacketHeader) && m_read_header.length < 8192) {
                    uint16_t body_length = m_read_header.length - sizeof(PacketHeader);
                    do_read_body(body_length);
                }
                else {
                    std::cerr << "Erro: Tamanho de pacote invalido (" << m_read_header.length << "). Desconectando." << std::endl;
                    close();
                }
            }
            else {
                // Erros comuns aqui s�o 'connection reset' ou 'end of file',
                // indicando que o cliente desconectou.
                if (ec != boost::asio::error::eof) {
                    std::cerr << "Erro ao ler o cabecalho: " << ec.message() << ". Desconectando." << std::endl;
                }
                close();
            }
        });
}

void ClientSession::do_read_body(uint16_t body_length) {
    m_read_body.resize(body_length);
    auto self = shared_from_this();
    boost::asio::async_read(m_socket,
        boost::asio::buffer(m_read_body.data(), body_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                // TODO: Descriptografar o corpo do pacote se necess�rio.

                // Pacote recebido com sucesso, vamos despach�-lo para processamento.
                Packet received_packet(m_read_header.opcode, m_read_body);
                m_packet_dispatcher.dispatch(self, received_packet);

                // Continua o ciclo de leitura, esperando pelo pr�ximo cabe�alho.
                do_read_header();
            }
            else {
                std::cerr << "Erro ao ler o corpo do pacote: " << ec.message() << ". Desconectando." << std::endl;
                close();
            }
        });
}

void ClientSession::send(const Packet& packet) {
    // Adiciona o pacote na fila de escrita.
    m_write_queue.push_back(packet);

    // Se a fila estava vazia, significa que n�o h� uma opera��o de escrita
    // em andamento, ent�o precisamos iniciar uma.
    if (m_write_queue.size() == 1) {
        do_write();
    }
}

void ClientSession::do_write() {
    auto self = shared_from_this();

    // Prepara os buffers para envio. Um para o cabe�alho, outro para o corpo.
    // Boost.Asio pode enviar m�ltiplos buffers em uma �nica chamada (scatter-gather I/O).
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&m_write_queue.front().header, sizeof(PacketHeader)));
    buffers.push_back(boost::asio::buffer(m_write_queue.front().body));

    boost::asio::async_write(m_socket, buffers,
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                // Pacote enviado com sucesso, remove da fila.
                m_write_queue.pop_front();

                // Se ainda houver pacotes na fila, continua o ciclo de escrita.
                if (!m_write_queue.empty()) {
                    do_write();
                }
            }
            else {
                std::cerr << "Erro ao escrever pacote: " << ec.message() << ". Desconectando." << std::endl;
                close();
            }
        });
}

void ClientSession::close() {
    // Encerra a conex�o de forma graciosa.
    // O socket ser� fechado pelo destrutor quando o �ltimo shared_ptr sair de escopo.
    boost::system::error_code ec;
    if (m_socket.is_open()) {
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
}