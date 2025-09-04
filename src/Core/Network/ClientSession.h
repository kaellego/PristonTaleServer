#pragma once

#include "Network/Packet.h"
#include "Network/PacketCipher.h"
#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <vector>
#include <string> // Necessário para std::string

// Forward declarations
class PacketDispatcher;
class LogService;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::ip::tcp::socket socket, PacketDispatcher& dispatcher, LogService& logService, uint8_t xorKey);
    ~ClientSession();

    void start();
    void send(Packet packet);
    void close();
    boost::asio::ip::tcp::socket& socket();

    // --- NOVOS MÉTODOS DE AUTENTICAÇÃO ---

    /**
     * @brief Define a sessão como autenticada, armazenando os dados da conta.
     * @param accountId O ID numérico da conta.
     * @param accountName O nome de usuário da conta.
     */
    void authenticate(int accountId, const std::string& accountName);

    bool isAuthenticated() const { return m_isAuthenticated; }
    int getAccountId() const { return m_accountId; }
    const std::string& getAccountName() const { return m_accountName; }

private:
    void do_read_header();
    void do_read_body();
    void do_write();
    void sendEncryptionKey();

    // --- NOVOS MEMBROS DE ESTADO ---
    bool m_isAuthenticated = false;
    int m_accountId = -1;
    std::string m_accountName;

    // Outros membros...
    boost::asio::ip::tcp::socket m_socket;
    PacketDispatcher& m_packet_dispatcher;
    LogService& m_logService;
    PacketCipher m_cipher;
    uint8_t m_xorKey;
    std::vector<uint8_t> m_write_buffer;
    PacketHeader m_read_header;
    std::vector<uint8_t> m_read_body;
    std::vector<uint8_t> m_keySetBuffer;
    std::deque<Packet> m_write_queue;
};