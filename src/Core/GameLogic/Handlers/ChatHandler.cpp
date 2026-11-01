#include "GameLogic/Handlers/ChatHandler.h"
#include "GameLogic/GameWorld.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"
#include "Shared/Constants.h"

#include <cstring>

void ChatHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    if (!session->isAuthenticated() || session->ticketExpired() || session->getCharacterName().empty() ||
        packet.body.size() != sizeof(uint32_t) + 256)
        return;

    if (std::memchr(packet.body.data() + sizeof(uint32_t), '\0', 256) == nullptr)
        return;

    const auto entityId = m_world.entityIdForAccount(session->getAccountName());
    if (!entityId)
        return;

    m_world.broadcastNearby(session->getAccountName(), *entityId, packet);
    m_logService.debug("Chat {}: {}", session->getCharacterName(),
                       reinterpret_cast<const char*>(packet.body.data() + sizeof(uint32_t)));
}
