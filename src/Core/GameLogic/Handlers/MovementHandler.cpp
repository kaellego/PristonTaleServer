#include "GameLogic/Handlers/MovementHandler.h"
#include "GameLogic/GameWorld.h"
#include "Network/ClientSession.h"
#include "Shared/Constants.h"
#include "Logging/LogService.h"
#include <cstring>
#include <cmath>

void MovementHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    if (!session->isAuthenticated() || session->ticketExpired() || session->getCharacterName().empty() || packet.body.size() < 16) { session->close(); return; }
    const auto entityId = m_world.entityIdForAccount(session->getAccountName());
    if (!entityId) { session->close(); return; }

    // smPLAYDATA body: dwObjectSerial, x, y, z, followed by legacy movement fields.
    const auto objectSerial = *reinterpret_cast<const int32_t*>(packet.body.data());
    const auto x = *reinterpret_cast<const int32_t*>(packet.body.data() + 4);
    const auto y = *reinterpret_cast<const int32_t*>(packet.body.data() + 8);
    const auto z = *reinterpret_cast<const int32_t*>(packet.body.data() + 12);
    if (objectSerial != *entityId || !std::isfinite(static_cast<double>(x)) || !std::isfinite(static_cast<double>(y)) || !std::isfinite(static_cast<double>(z))) { session->close(); return; }

    const MapPosition destination{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    if (!m_world.moveEntity(*entityId, destination)) return;

    auto body = packet.body;
    std::memcpy(body.data(), &*entityId, sizeof(int32_t));
    Packet update(static_cast<uint32_t>(Opcodes::UnitStatusMove), body);
    m_world.broadcastNearby(session->getAccountName(), *entityId, update);
}
