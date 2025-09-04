#include "Network/ClientSession.h"
#include "Network/PacketDispatcher.h"
#include "Logging/LogService.h"
#include "Network/PacketScrambler.h"
#include "Shared/Constants.h"
#include <iostream>
#include <utility>
#include <iomanip> // Para logs hexadecimais, se necessário

ClientSession::ClientSession(boost::asio::ip::tcp::socket socket, PacketDispatcher& dispatcher, LogService& logService, uint8_t xorKey)
    : m_socket(std::move(socket)),
    m_packet_dispatcher(dispatcher),
    m_logService(logService),
    m_xorKey(xorKey)
{}

ClientSession::~ClientSession() {
    if (m_socket.is_open()) {
        try { m_logService.info("Sessao encerrada para {}:{}", m_socket.remote_endpoint().address().to_string(), m_socket.remote_endpoint().port() ); }
        catch (...) {}
    }
    else {
        m_logService.info("Sessao encerrada para um socket ja desconectado.");
    }
}

boost::asio::ip::tcp::socket& ClientSession::socket() {
    return m_socket;
}

void ClientSession::start() {
    try {
        m_logService.info("Nova sessao iniciada: {}:{}", m_socket.remote_endpoint().address().to_string(), m_socket.remote_endpoint().port() );
    }
    catch (...) { return; }

    sendEncryptionKey();
    // A chamada correta para iniciar o ciclo de leitura
    do_read_header();
}

void ClientSession::sendEncryptionKey() {
    uint16_t port = 0;
    try {
        port = m_socket.local_endpoint().port();
    }
    catch (...) {}

    m_keySetBuffer = m_cipher.generateKeySetPacket(port);
    auto self = shared_from_this();

    boost::asio::async_write(m_socket, boost::asio::buffer(m_keySetBuffer),
        [self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                self->m_logService.packet("Pacote de chave enviado com sucesso ({} bytes).", bytes_transferred);
            }
            else {
                self->m_logService.warn("Falha ao enviar o pacote de chave: {}", ec.message());
                self->close();
            }
        });
}

void ClientSession::do_read_header() {
    auto self = shared_from_this();
    boost::asio::async_read(m_socket,
        boost::asio::buffer(&m_read_header, sizeof(PacketHeader)),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                PacketScrambler::scramble(m_read_header, m_xorKey);

                if (m_read_header.length >= sizeof(PacketHeader) && m_read_header.length < 8192) {
                    do_read_body();
                }
                else {
                    // Log com o valor "scrambled", pois ainda não descriptografamos
                    m_logService.error("Tamanho de pacote (scrambled) invalido: {}. Desconectando.", m_read_header.length);
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

void ClientSession::do_read_body() {
    uint16_t body_length = m_read_header.length - sizeof(PacketHeader);
    m_read_body.resize(body_length);

    auto self = shared_from_this();
    boost::asio::async_read(m_socket,
        boost::asio::buffer(m_read_body.data(), body_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                PacketScrambler::scramble(m_read_body, m_xorKey);

                Packet received_packet(m_read_header, m_read_body);
                m_cipher.decrypt(received_packet);

                if (received_packet.header.length != (sizeof(PacketHeader) + received_packet.body.size())) {
                    m_logService.error("Incompatibilidade de tamanho do pacote apos descriptografar. Desconectando.");
                    close();
                    return;
                }

                // --- A NOVA LÓGICA DE LOG DETALHADO ---
                std::string opcodeName = getOpcodeName(received_packet.header.opcode);
                std::string hexDump = formatHex(received_packet.body);

                m_logService.packet("-> Pacote Recebido: {} (Opcode: 0x{:04x}, Tamanho: {}) | Body: {}",
                    opcodeName,
                    received_packet.header.opcode,
                    received_packet.header.length,
                    hexDump);

                m_packet_dispatcher.dispatch(self, received_packet);

                do_read_header();
            }
            else {
                m_logService.warn("Erro ao ler o corpo do pacote: {}. Desconectando.", ec.message());
                close();
            }
        });
}

void ClientSession::send(Packet packet) {
    m_cipher.encrypt(packet);

    auto self = shared_from_this();
    boost::asio::post(m_socket.get_executor(), [this, self, packet = std::move(packet)]() mutable {
        bool write_in_progress = !m_write_queue.empty();
        m_write_queue.push_back(std::move(packet));
        if (!write_in_progress) {
            do_write();
        }
        });
}

void ClientSession::do_write() {
    auto self = shared_from_this();

    // 1. Prepara o buffer de escrita com os dados do pacote da fila
    const auto& packet_to_send = m_write_queue.front();
    size_t packet_size = sizeof(PacketHeader) + packet_to_send.body.size();
    m_write_buffer.resize(packet_size);

    std::memcpy(m_write_buffer.data(), &packet_to_send.header, sizeof(PacketHeader));
    if (!packet_to_send.body.empty()) {
        std::memcpy(m_write_buffer.data() + sizeof(PacketHeader), packet_to_send.body.data(), packet_to_send.body.size());
    }

    // 2. APLICA O SCRAMBLE NO BUFFER COMPLETO ANTES DE ENVIAR
    PacketScrambler::scramble(m_write_buffer, m_xorKey);

    boost::asio::async_write(m_socket, boost::asio::buffer(m_write_buffer),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                m_write_queue.pop_front();
                if (!m_write_queue.empty()) {
                    do_write();
                }
            }
            else {
                m_logService.warn("Erro ao escrever pacote: {}. Desconectando.", ec.message());
                close();
            }
        });
}

void ClientSession::close() {
    boost::system::error_code ec;
    if (m_socket.is_open()) {
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
}