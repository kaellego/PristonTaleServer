#include "Network/PacketDispatcher.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"
#include "GameLogic/Handlers/PingHandler.h"
#include <iostream>

constexpr uint16_t OPCODE_PING = 0x07E6;

PacketDispatcher::PacketDispatcher(GlobalState& state, PlayerRepository& playerRepo, ItemRepository& itemRepo) :
    m_globalState(state),
    m_playerRepository(playerRepo),
    m_itemRepository(itemRepo)
{
    std::cout << "Registrando manipuladores de pacotes..." << std::endl;
    registerHandlers();
    std::cout << m_handlers.size() << " manipuladores de pacotes registrados." << std::endl;
}

void PacketDispatcher::registerHandlers() {
    m_handlers[OPCODE_PING] = std::make_unique<PingHandler>();
}

void PacketDispatcher::dispatch(std::shared_ptr<ClientSession> session, const Packet& packet) {
    uint16_t opcode = packet.header.opcode;
    auto it = m_handlers.find(opcode);

    if (it != m_handlers.end()) {
        it->second->handle(session, packet);
    }
    else {
        try {
            std::cout << "AVISO: Pacote nao manipulado recebido do cliente "
                << session->socket().remote_endpoint()
                << ". Opcode: 0x" << std::hex << opcode << std::dec << std::endl;
        }
        catch (...) {
            // Ignora exceções se o socket já estiver fechado.
        }
    }
}