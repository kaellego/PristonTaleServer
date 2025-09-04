#pragma once

#include "Network/Packet.h"
#include "Network/PacketCipher.h"
#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <vector>

// Forward declarations
class PacketDispatcher;
class LogService;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::ip::tcp::socket socket, PacketDispatcher& dispatcher, LogService& logService, uint8_t xorKey);
    ~ClientSession();

    void start();
    void send(Packet packet);
    boost::asio::ip::tcp::socket& socket();
    void close();

private:
    // A lógica final e correta de leitura
    void do_read_header();
    void do_read_body();

    void do_write();
    void sendEncryptionKey();

    boost::asio::ip::tcp::socket m_socket;
    PacketDispatcher& m_packet_dispatcher;
    LogService& m_logService;
    PacketCipher m_cipher;

    uint8_t m_xorKey;

    // Buffers
    PacketHeader m_read_header;
    std::vector<uint8_t> m_read_body;
    std::vector<uint8_t> m_keySetBuffer;
    std::vector<uint8_t> m_write_buffer;
    std::deque<Packet> m_write_queue;
};