#include "GameLogic/Handlers/MovementHandler.h"
#include "GameLogic/GameWorld.h"
#include "Network/ClientSession.h"
#include "Shared/Constants.h"
#include "Logging/LogService.h"
#include <cstring>
#include <cmath>
#include <optional>
#include <vector>

void MovementHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    if (!session->isAuthenticated() || session->ticketExpired() || packet.body.size() < 16) { session->close(); return; }
    const auto entityId = m_world.entityIdForAccount(session->getAccountName());
    // The legacy client opens the GameServer socket before the character is
    // selected and sends an initial position during that handshake. Ignore it
    // until the authenticated character has been inserted into GameWorld.
    if (!entityId) {
        m_logService.debug("Movimento inicial ignorado antes da selecao: conta={}", session->getAccountName());
        return;
    }

    const bool legacyPosition = packet.header.opcode == static_cast<uint32_t>(Opcodes::LegacyPosition);
    int32_t x = 0, y = 0, z = 0, area = -1;
    uint32_t objectSerial = 0;
    const size_t xOffset = legacyPosition ? 0u : sizeof(int32_t);
    std::memcpy(&x, packet.body.data() + xOffset, sizeof(x));
    std::memcpy(&y, packet.body.data() + xOffset + sizeof(int32_t), sizeof(y));
    std::memcpy(&z, packet.body.data() + xOffset + sizeof(int32_t) * 2, sizeof(z));
    if (legacyPosition) {
        std::memcpy(&area, packet.body.data() + sizeof(int32_t) * 3, sizeof(area));
    } else {
        std::memcpy(&objectSerial, packet.body.data(), sizeof(objectSerial));
        if (!m_world.ownsObjectSerial(*entityId, objectSerial)) { session->close(); return; }
    }
    if (!std::isfinite(static_cast<double>(x)) || !std::isfinite(static_cast<double>(y)) ||
        !std::isfinite(static_cast<double>(z))) { session->close(); return; }

    const MapPosition destination{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    const auto mapId = area >= 0 ? std::optional<int>(area) : std::nullopt;
    if (!m_world.moveEntity(*entityId, destination, mapId)) return;

    const auto entity = m_world.entity(*entityId);
    if (!entity) return;
    std::vector<uint8_t> body(sizeof(int32_t) * 4);
    const auto serial = entity->objectSerial;
    const auto positionX = static_cast<int32_t>(entity->position.x);
    const auto positionY = static_cast<int32_t>(entity->position.y);
    const auto positionZ = static_cast<int32_t>(entity->position.z);
    std::memcpy(body.data(), &serial, sizeof(serial));
    std::memcpy(body.data() + 4, &positionX, sizeof(positionX));
    std::memcpy(body.data() + 8, &positionY, sizeof(positionY));
    std::memcpy(body.data() + 12, &positionZ, sizeof(positionZ));
    m_logService.debug("Movimento persistido: conta={} mapa={} x={} y={} z={}",
                       session->getAccountName(), entity->mapId, positionX, positionY, positionZ);
    Packet update(static_cast<uint32_t>(Opcodes::UnitStatusMove), std::move(body));
    m_world.broadcastNearby(session->getAccountName(), *entityId, update);
}
