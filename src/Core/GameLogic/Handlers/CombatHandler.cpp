#include "GameLogic/Handlers/CombatHandler.h"
#include "GameLogic/GameWorld.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"
#include "Shared/Constants.h"
#include <cstdint>
#include <cstring>

namespace
{
#pragma pack(push, 1)
    struct LegacyAttackData2Body
    {
        uint32_t checkSum;
        uint32_t destObjectSerial, targetObjectSerial;
        int32_t x, y, z;
        int32_t attackState, attackSize;
        int16_t power[2], power2[2], critical[2];
        int32_t skillCode;
        uint32_t time;
        int32_t attackCount;
        int16_t motionCount[2];
        uint32_t weaponCode;
        int16_t area[2];
        int32_t temp[4];
    };
#pragma pack(pop)

    static_assert(sizeof(LegacyAttackData2Body) == 84);
}

void CombatHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    if (!session->isAuthenticated() || session->ticketExpired() || session->getCharacterName().empty() || packet.body.size() < 8) { session->close(); return; }
    const auto ownEntity = m_world.entityIdForAccount(session->getAccountName());
    const auto entity = m_world.entityForAccount(session->getAccountName());
    if (!ownEntity || !entity) return;

    CombatResult result{};
    uint32_t target = 0;
    if (packet.body.size() == sizeof(LegacyAttackData2Body))
    {
        LegacyAttackData2Body attack{};
        std::memcpy(&attack, packet.body.data(), sizeof(attack));
        if (attack.destObjectSerial != entity->objectSerial || attack.targetObjectSerial == 0 ||
            attack.attackSize < 0 || attack.attackCount < 0 || attack.attackCount > 256)
        {
            m_logService.debug("ATTACKDATA2 rejeitado: serial/alvos/limites invalidos para {}", session->getAccountName());
            return;
        }
        target = attack.targetObjectSerial;
        result = m_world.attackNpc(*ownEntity, target);
    }
    else
    {
        uint32_t attacker = 0;
        std::memcpy(&attacker, packet.body.data(), sizeof(attacker));
        std::memcpy(&target, packet.body.data() + sizeof(attacker), sizeof(target));
        if (attacker != static_cast<uint32_t>(*ownEntity) || target == attacker) return;
        result = m_world.attack(*ownEntity, static_cast<int>(target));
    }
    if (!result.hit) return;
    const int32_t damage = result.damage;
    const std::vector<uint8_t> damageBody(reinterpret_cast<const uint8_t*>(&damage),
                                           reinterpret_cast<const uint8_t*>(&damage) + sizeof(damage));
    session->send(Packet(static_cast<uint32_t>(Opcodes::LastAttackDamage), damageBody));
    session->send(Packet(static_cast<uint32_t>(Opcodes::AttackResult),
                         std::vector<uint8_t>{static_cast<uint8_t>(result.targetDefeated ? 1 : 0), 0, 0, 0}));
    // The legacy client expects the original attack command to be visible to nearby units.
    m_world.broadcastNearby(session->getAccountName(), *ownEntity, packet);
    m_logService.debug("Ataque 0x{:08x} -> 0x{:08x}: dano={}, vida={}, derrotado={}",
                       entity->objectSerial, target, result.damage, result.targetHealth, result.targetDefeated);
}
