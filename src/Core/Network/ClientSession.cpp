#include "Network/ClientSession.h"
#include "Network/PacketDispatcher.h"
#include "Logging/LogService.h"
#include "Network/PacketScrambler.h"
#include "Network/LegacyPacketCodec.h"
#include "Network/LegacyGatewayProtocol.h"
#include "Shared/Constants.h"
#include <iostream>
#include <utility>
#include <cstring>
#include <iomanip>

ClientSession::ClientSession(boost::asio::ip::tcp::socket socket, PacketDispatcher& dispatcher, LogService& logService, uint8_t xorKey, bool gameEndpoint)
    : m_socket(std::move(socket)),
    m_strand(boost::asio::make_strand(m_socket.get_executor())),
    m_packet_dispatcher(dispatcher),
    m_logService(logService),
    m_xorKey(xorKey),
    m_gameEndpoint(gameEndpoint)
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
        m_logService.info("Nova sessao iniciada: {}:{} (local {})", m_socket.remote_endpoint().address().to_string(), m_socket.remote_endpoint().port(), m_socket.local_endpoint().port());
    }
    catch (...) { return; }

    // O cliente legado não entende o pacote de chave moderno. O gateway
    // começa pelo frame legado e só encaminha payloads validados.
    m_logService.debug("Sessao::start() <-> Aguardando frame legado");
    m_logService.info("Enviando confirmacao legada VERSION para {}", m_gameEndpoint ? "GameServer" : "LoginServer");
    m_legacy_write_queue.push_back(LegacyGateway::gameServerReadyPayload());
    do_legacy_write();
    do_read_legacy_length();
}

void ClientSession::do_read_legacy_length() {
    auto self = shared_from_this();
    boost::asio::async_read(m_socket, boost::asio::buffer(m_legacyLengthBuffer),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (ec) { close(); return; }
            uint32_t length = 0;
            std::memcpy(&length, m_legacyLengthBuffer.data(), sizeof(length));
            if (length < sizeof(LegacyGateway::Command) || length > LegacyPacketCodec::MaxFrameSize) {
                m_logService.warn("Frame legado invalido: {}", length);
                close();
                return;
            }
            m_legacyFrame.assign(length, 0);
            std::memcpy(m_legacyFrame.data(), m_legacyLengthBuffer.data(), sizeof(length));
            do_read_legacy_frame();
        });
}

void ClientSession::do_read_legacy_frame() {
    auto self = shared_from_this();
    boost::asio::async_read(m_socket,
        boost::asio::buffer(m_legacyFrame.data() + sizeof(uint32_t), m_legacyFrame.size() - sizeof(uint32_t)),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (ec) { close(); return; }

            auto packet = LegacyGateway::toModernPacket(m_legacyFrame);
            if (!packet) {
                int32_t legacyCode = 0;
                std::memcpy(&legacyCode, m_legacyFrame.data() + sizeof(int32_t), sizeof(legacyCode));
                m_logService.warn("Frame legado nao suportado: size={}, code={}, bytes={}",
                    m_legacyFrame.size(), legacyCode,
                    m_legacyFrame.size() >= 8 ? std::to_string(m_legacyFrame[0]) + " " + std::to_string(m_legacyFrame[1]) + " " + std::to_string(m_legacyFrame[2]) + " " + std::to_string(m_legacyFrame[3]) : "");
                close();
                return;
            }
            std::memcpy(&m_legacyRequestCode, m_legacyFrame.data() + sizeof(int32_t), sizeof(m_legacyRequestCode));
            if (m_gameEndpoint && !isAuthenticated()) {
                bool transferred = false;
                if (m_legacyRequestCode == 10 || m_legacyRequestCode == 11) {
                    char account[21]{};
                    std::memcpy(account, m_legacyFrame.data() + 20, sizeof(account));
                    transferred = m_packet_dispatcher.transferLegacySession(self, account);
                    if (!transferred)
                        m_logService.warn("Sessao legada do GameServer rejeitada para a conta '{}'.", account);
                } else {
                    transferred = m_packet_dispatcher.transferLegacySessionByIp(self);
                }
                if (!transferred) {
                    m_logService.warn("Sessao legada do GameServer rejeitada: conta nao identificada.");
                    close();
                    return;
                }
            }
            m_packet_dispatcher.dispatch(self, *packet);
            do_read_legacy_length();
        });
}

void ClientSession::sendEncryptionKey() {
    m_logService.debug("Sessao::sendEncryptionKey() <-> Gerando e enviando pacote de chave...");
    uint16_t port = 0;
    try {
        port = m_socket.local_endpoint().port();
    }
    catch (...) {}

    m_keySetBuffer = m_cipher.generateKeySetPacket(port);
    auto self = shared_from_this();

    boost::asio::async_write(m_socket, boost::asio::buffer(m_keySetBuffer),
        [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            m_logService.debug("Sessao::sendEncryptionKey() <-> Callback de envio executado.");
            if (!ec) {
                m_logService.packet("Pacote de chave enviado com sucesso ({} bytes).", bytes_transferred);
                //self->m_logService.packet("Pacote de chave enviado com sucesso ({} bytes).", bytes_transferred);
                
                m_logService.debug("Sessao::sendEncryptionKey() <-> Chamando do_read_header()");
                do_read_header();
            }
            else {
                m_logService.warn("Falha ao enviar o pacote de chave: {}", ec.message());
                //self->m_logService.warn("Falha ao enviar o pacote de chave: {}", ec.message());
                close();
            }
        });
}

void ClientSession::do_read_header() {
    m_logService.debug("Sessao::do_read_header() <-> Aguardando por dados...");
    auto self = shared_from_this();
    boost::asio::async_read(m_socket,
        boost::asio::buffer(&m_read_header, sizeof(PacketHeader)),
        [this, self](boost::system::error_code ec, std::size_t length) {
            m_logService.debug("Sessao::do_read_header() <-> Callback de leitura executado.");
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

                m_logService.packet("<- Pacote Recebido: {} (Opcode: 0x{:04x}, Tamanho: {}) | Body: {}",
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
    if (auto legacy = LegacyGateway::toLegacyPayload(packet, m_legacySendCounter + 1, m_legacyRequestCode); legacy) {
        if (legacy->empty()) return; // Successful legacy login has no response frame.
        m_legacySendCounter++;
        auto frame = std::move(*legacy);
        auto self = shared_from_this();
        boost::asio::post(m_strand, [this, self, frame = std::move(frame)]() mutable {
            const bool writing = !m_legacy_write_queue.empty();
            m_legacy_write_queue.push_back(std::move(frame));
            if (!writing) do_legacy_write();
        });
        return;
    }
    m_cipher.encrypt(packet);

    auto self = shared_from_this();
    boost::asio::post(m_strand, [this, self, packet = std::move(packet)]() mutable {
        bool write_in_progress = !m_write_queue.empty();
        m_write_queue.push_back(std::move(packet));
        if (!write_in_progress) {
            do_write();
        }
        });
}

void ClientSession::do_legacy_write() {
    auto self = shared_from_this();
    boost::asio::async_write(m_socket, boost::asio::buffer(m_legacy_write_queue.front()),
        boost::asio::bind_executor(m_strand, [this, self](boost::system::error_code ec, std::size_t) {
            if (ec) { close(); return; }
            m_legacy_write_queue.pop_front();
            if (!m_legacy_write_queue.empty()) do_legacy_write();
        }));
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
        boost::asio::bind_executor(m_strand,
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
        }));
}

void ClientSession::close() {
    if (m_closeStarted.exchange(true)) return;
    m_packet_dispatcher.sessionClosed(shared_from_this());
    boost::system::error_code ec;
    if (m_socket.is_open()) {
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
}
void ClientSession::authenticate(int accountId, const std::string& accountName, int ticket, std::chrono::steady_clock::time_point expiresAt) {
    m_isAuthenticated = true;
    m_accountId = accountId;
    m_accountName = accountName;
    m_ticket = ticket;
    m_ticketExpiresAt = expiresAt;
}
