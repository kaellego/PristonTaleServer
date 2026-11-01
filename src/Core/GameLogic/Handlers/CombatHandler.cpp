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
    const int32_t damage = result.damage;
    const std::vector<uint8_t> damageBody(reinterpret_cast<const uint8_t*>(&damage),
                                           reinterpret_cast<const uint8_t*>(&damage) + sizeof(damage));
    session->send(Packet(static_cast<uint32_t>(Opcodes::LastAttackDamage), damageBody));
    session->send(Packet(static_cast<uint32_t>(Opcodes::AttackResult),
                         std::vector<uint8_t>{static_cast<uint8_t>(result.targetDefeated ? 1 : 0), 0, 0, 0}));
    // The legacy client expects the original attack command to be visible to nearby units.
    m_world.broadcastNearby(session->getAccountName(), *ownEntity, packet);
    m_logService.debug("Ataque {} -> {}: dano={}, vida={}, derrotado={}", attacker, target, result.damage, result.targetHealth, result.targetDefeated);
}
