#include "GameLogic/Handlers/CharacterHandler.h"
#include "GameLogic/Services/CharacterService.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "Logging/LogService.h"
#include <cstring>
CharacterHandler::CharacterHandler(CharacterService& service, LogService& logService) : m_service(service), m_logService(logService) {}
void CharacterHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    if (!session->isAuthenticated() || packet.body.size() != sizeof(PacketCharacterCommand) - sizeof(PacketHeader)) { session->close(); return; }
    PacketCharacterCommand command{};
    std::memcpy(reinterpret_cast<uint8_t*>(&command) + sizeof(PacketHeader), packet.body.data(), packet.body.size());
    const std::string name(command.szName, strnlen_s(command.szName, sizeof(command.szName)));
    bool success = false;
    switch (static_cast<Opcodes>(packet.header.opcode)) {
    case Opcodes::CreateCharacter: success = m_service.createCharacter(session->getAccountName(), name, command.LParam); break;
    case Opcodes::DeleteCharacter: success = m_service.deleteCharacter(session->getAccountName(), name); break;
    case Opcodes::SelectCharacter: success = m_service.selectCharacter(session->getAccountName(), name).has_value(); break;
    default: return;
    }
    PacketRecordResult result{};
    result.header.opcode = static_cast<uint32_t>(success ? Opcodes::RecordResult : Opcodes::FailRecordData);
    result.Result = success ? 1 : 0;
    strncpy_s(result.szName, sizeof(result.szName), name.c_str(), _TRUNCATE);
    const auto* body = reinterpret_cast<const uint8_t*>(&result) + sizeof(PacketHeader);
    session->send(Packet(result.header.opcode, std::vector<uint8_t>(body, body + sizeof(result) - sizeof(PacketHeader))));
    m_logService.info("Operacao de personagem {} para {}: {}", success ? "aceita" : "rejeitada", session->getAccountName(), name);
}
