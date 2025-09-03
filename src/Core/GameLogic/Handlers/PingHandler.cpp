#include "GameLogic/Handlers/PingHandler.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"

// Opcodes
constexpr uint16_t OPCODE_PING_PONG = 0x03EC;

void PingHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    // Lógica do Ping: simplesmente envia um pacote de Pong de volta.
    // Corpo do pacote de resposta está vazio.
    std::vector<uint8_t> responseBody;
    Packet pongPacket(OPCODE_PING_PONG, responseBody);

    session->send(pongPacket);
}