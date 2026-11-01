#include "GameLogic/Handlers/CombatHandler.h"
#include "GameLogic/GameWorld.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"
#include "Shared/Constants.h"
#include <cstdint>
#include <cstring>

void CombatHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    if (!session->isAuthenticated() || session->ticketExpired() || session->getCharacterName().empty() || packet.body.size() < 8) { session->close(); return; }
    const auto attacker = *reinterpret_cast<const uint32_t*>(packet.body.data());
    const auto target = *reinterpret_cast<const uint32_t*>(packet.body.data() + 4);
    const auto ownEntity = m_world.entityIdForAccount(session->getAccountName());
    if (!ownEntity || attacker != static_cast<uint32_t>(*ownEntity) || target == attacker) return;
    const auto result = m_world.attack(*ownEntity, static_cast<int>(target));
    if (!result.hit) return;
    // The legacy client expects the original attack command to be visible to nearby units.
    m_world.broadcastNearby(session->getAccountName(), *ownEntity, packet);
    m_logService.debug("Ataque {} -> {}: dano={}, vida={}, derrotado={}", attacker, target, result.damage, result.targetHealth, result.targetDefeated);
}
