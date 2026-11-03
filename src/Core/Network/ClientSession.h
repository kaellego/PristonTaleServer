#pragma once

#include "Network/Packet.h"
#include "Network/LegacyPacketAdapter.h"
#include "Network/PacketCipher.h"
#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <vector>
#include <atomic>
#include <array>
#include <chrono>
#include <string> // Necessario para std::string
#include <unordered_map>
#include <mutex>

// Forward declarations
class PacketDispatcher;
class LogService;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::ip::tcp::socket socket, PacketDispatcher& dispatcher, LogService& logService, uint8_t xorKey, bool gameEndpoint = false);
    ~ClientSession();

    void start();
    void send(Packet packet);
    // Refreshes that announce legacy objects must choose their wire context.
    void sendLegacyPacket(Packet packet, int32_t requestCode);
    void close();
    boost::asio::ip::tcp::socket& socket();

    // --- NOVOS MÃ‰TODOS DE AUTENTICAÃ‡ÃƒO ---

    /**
     * @brief Define a sessÃ£o como autenticada, armazenando os dados da conta.
     * @param accountId O ID numÃ©rico da conta.
     * @param accountName O nome de usuÃ¡rio da conta.
     */
    void authenticate(int accountId, const std::string& accountName, int ticket, std::chrono::steady_clock::time_point expiresAt);

    bool isAuthenticated() const { return m_isAuthenticated; }
    int getAccountId() const { return m_accountId; }
    const std::string& getAccountName() const { return m_accountName; }
    void selectCharacter(const std::string& name) { m_characterName = name; }
    const std::string& getCharacterName() const { return m_characterName; }
    void markVerifiedLogin() { ++m_verifiedLoginPackets; }
    uint32_t getVerifiedLoginPackets() const { return m_verifiedLoginPackets; }
    uint32_t clientObjectSerial() const { return m_clientObjectSerial; }
    void setClientObjectSerial(uint32_t serial) { m_clientObjectSerial = serial; }
    int getTicket() const { return m_ticket; }
    bool ticketExpired() const { return std::chrono::steady_clock::now() >= m_ticketExpiresAt; }
    void renewTicket(std::chrono::steady_clock::time_point expiresAt) { m_ticketExpiresAt = expiresAt; }
    bool acceptItemOperation(uint32_t opcode, const std::vector<uint8_t>& body);
    enum class InboundFrameDecision { Accept, Drop, Close };
    InboundFrameDecision acceptInboundFrame(uint32_t opcode, std::size_t bytes);
    bool shouldLogKeepAlive();
    bool shouldLogRateViolation();
    void setNpcContext(int npcId, uint32_t serial) { m_npcId = npcId; m_npcSerial = serial; }
    int npcContextId() const { return m_npcId; }
    uint32_t npcContextSerial() const { return m_npcSerial; }

private:
    void do_read_header();
    void do_read_body();
    void do_read_legacy_length();
    void do_read_legacy_frame();
    void do_write();
    void do_legacy_write();
    void sendEncryptionKey();

    // --- NOVOS MEMBROS DE ESTADO ---
    bool m_isAuthenticated = false;
    std::atomic_bool m_closeStarted{false};
    int m_accountId = -1;
    std::string m_accountName;
    int m_ticket = 0;
    std::chrono::steady_clock::time_point m_ticketExpiresAt{};
    std::string m_characterName;
    int m_npcId = 0;
    uint32_t m_npcSerial = 0;
    uint32_t m_verifiedLoginPackets = 0;
    uint32_t m_clientObjectSerial = 0;
    std::mutex m_operationMutex;
    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> m_recentItemOperations;
    std::mutex m_rateMutex;
    std::chrono::steady_clock::time_point m_rateWindowStart{};
    std::chrono::steady_clock::time_point m_lastKeepAliveLog{};
    std::chrono::steady_clock::time_point m_lastRateViolationLog{};
    uint32_t m_inboundFramesInWindow = 0;
    uint32_t m_keepAlivesInWindow = 0;

    // Outros membros...
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::strand<boost::asio::any_io_executor> m_strand;
    PacketDispatcher& m_packet_dispatcher;
    LogService& m_logService;
    PacketCipher m_cipher;
    uint8_t m_xorKey;
    bool m_gameEndpoint;
    int32_t m_legacyRequestCode = 0;
    std::vector<uint8_t> m_write_buffer;
    PacketHeader m_read_header;
    std::vector<uint8_t> m_read_body;
    std::vector<uint8_t> m_keySetBuffer;
    std::array<uint8_t, 4> m_legacyLengthBuffer{};
    std::vector<uint8_t> m_legacyFrame;
    uint32_t m_legacyCounter = 0;
    LegacyPacketAdapter m_legacyAdapter;
    uint32_t m_legacySendCounter = 0;
    std::deque<Packet> m_write_queue;
    std::deque<std::vector<uint8_t>> m_legacy_write_queue;
};
