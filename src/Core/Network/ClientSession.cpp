#include "Network/ClientSession.h"
#include "Network/PacketDispatcher.h"
#include "Logging/LogService.h"
#include <iostream>

ClientSession::ClientSession(boost::asio::ip::tcp::socket socket, PacketDispatcher& dispatcher, LogService& logService)
    : m_socket(std::move(socket)), m_packet_dispatcher(dispatcher), m_logService(logService) {}

ClientSession::~ClientSession() {
    // Log para depuração, para sabermos quando uma sessão é destruída.
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
        return; // Não podemos continuar se o socket já estiver fechado.
    }

    // Inicia o ciclo de leitura assíncrona.
    do_read_header();
}

void ClientSession::do_read_header() {
    auto self = shared_from_this();
    boost::asio::async_read(m_socket,
        boost::asio::buffer(&m_read_header, sizeof(PacketHeader)),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                // --- NOSSO NOVO CÓDIGO DE DEBUG COMEÇA AQUI ---
                if (length > 0) {
                    std::stringstream hex_stream;
                    hex_stream << std::hex << std::setfill('0');

                    const uint8_t* data = reinterpret_cast<const uint8_t*>(&m_read_header);
                    for (size_t i = 0; i < length; ++i) {
                        hex_stream << std::setw(2) << static_cast<int>(data[i]) << " ";
                    }

                    // Usa o nível 'packet' do nosso LogService para registrar os dados brutos
                    m_logService.packet("Recebido cabecalho bruto ({} bytes): {}", length, hex_stream.str());
                }
                // --- FIM DO CÓDIGO DE DEBUG ---

                // A lógica original de validação continua aqui
                if (m_read_header.length >= sizeof(PacketHeader) && m_read_header.length < 8192) {
                    uint16_t body_length = m_read_header.length - sizeof(PacketHeader);
                    do_read_body(body_length);
                }
                else {
                    // Agora usamos o logService para registrar o erro de forma mais clara
                    m_logService.error("Tamanho de pacote invalido ({}). Desconectando cliente {}.", m_read_header.length, socket().remote_endpoint().address().to_string());
                    close();
                }
            }
            else {
                if (ec != boost::asio::error::eof) {
                    m_logService.warn("Erro ao ler o cabecalho: {}. Desconectando.", ec.message());
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
                // TODO: Descriptografar o corpo do pacote se necessário.

                // Pacote recebido com sucesso, vamos despachá-lo para processamento.
                Packet received_packet(m_read_header.opcode, m_read_body);
                m_packet_dispatcher.dispatch(self, received_packet);

                // Continua o ciclo de leitura, esperando pelo próximo cabeçalho.
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

    // Se a fila estava vazia, significa que não há uma operação de escrita
    // em andamento, então precisamos iniciar uma.
    if (m_write_queue.size() == 1) {
        do_write();
    }
}

void ClientSession::do_write() {
    auto self = shared_from_this();

    // Prepara os buffers para envio. Um para o cabeçalho, outro para o corpo.
    // Boost.Asio pode enviar múltiplos buffers em uma única chamada (scatter-gather I/O).
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
    // Encerra a conexão de forma graciosa.
    // O socket será fechado pelo destrutor quando o último shared_ptr sair de escopo.
    boost::system::error_code ec;
    if (m_socket.is_open()) {
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
}