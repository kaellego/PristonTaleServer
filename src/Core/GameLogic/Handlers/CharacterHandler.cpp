#include "GameLogic/Handlers/CharacterHandler.h"
#include "GameLogic/Services/CharacterService.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "Logging/LogService.h"
#include "GameLogic/GameWorld.h"
#include <cstring>
CharacterHandler::CharacterHandler(CharacterService &service, GameWorld& world, LogService &logService) : m_service(service), m_world(world), m_logService(logService) {}

namespace
{
    void sendCharacterList(const std::shared_ptr<ClientSession> &session, CharacterService &service)
    {
        PacketUserInfo response{};
        response.header.opcode = static_cast<uint32_t>(Opcodes::UserInfo);
        strncpy_s(response.szUserID, sizeof(response.szUserID), session->getAccountName().c_str(), _TRUNCATE);
        const auto characters = service.getCharacterList(session->getAccountName());
        response.CharCount = static_cast<int32_t>(characters.size());
        for (size_t i = 0; i < characters.size() && i < 6; ++i)
        {
            strncpy_s(response.sCharacterData[i].szName, sizeof(response.sCharacterData[i].szName), characters[i].szName, _TRUNCATE);
            response.sCharacterData[i].iLevel = characters[i].iLevel;
            response.sCharacterData[i].JobCode = characters[i].iClass;
        }
        const auto *body = reinterpret_cast<const uint8_t *>(&response) + sizeof(PacketHeader);
        session->send(Packet(response.header.opcode, std::vector<uint8_t>(body, body + sizeof(response) - sizeof(PacketHeader))));
    }
}

void CharacterHandler::handle(std::shared_ptr<ClientSession> session, const Packet &packet)
{
    if (!session->isAuthenticated() || session->ticketExpired() || packet.body.size() != sizeof(PacketCharacterCommand) - sizeof(PacketHeader))
    {
        session->close();
        return;
    }
    PacketCharacterCommand command{};
    std::memcpy(reinterpret_cast<uint8_t *>(&command) + sizeof(PacketHeader), packet.body.data(), packet.body.size());
    const std::string name(command.szName, strnlen_s(command.szName, sizeof(command.szName)));
    bool success = false;
    switch (static_cast<Opcodes>(packet.header.opcode))
    {
    case Opcodes::CreateCharacter:
        success = m_service.createCharacter(session->getAccountName(), name, command.LParam);
        break;
    case Opcodes::DeleteCharacter:
        success = m_service.deleteCharacter(session->getAccountName(), name);
        break;
    case Opcodes::SelectCharacter:
        success = m_service.selectCharacter(session->getAccountName(), name).has_value();
        if (success)
        {
            session->selectCharacter(name);
            m_world.enter(session->getAccountId(), session->getAccountName(), name, session);
        }
        break;
    default:
        return;
    }
    PacketRecordResult result{};
    result.header.opcode = static_cast<uint32_t>(success ? Opcodes::RecordResult : Opcodes::FailRecordData);
    result.Result = success ? 1 : 0;
    strncpy_s(result.szName, sizeof(result.szName), name.c_str(), _TRUNCATE);
    const auto *body = reinterpret_cast<const uint8_t *>(&result) + sizeof(PacketHeader);
    session->send(Packet(result.header.opcode, std::vector<uint8_t>(body, body + sizeof(result) - sizeof(PacketHeader))));
    if (success && static_cast<Opcodes>(packet.header.opcode) != Opcodes::SelectCharacter)
        sendCharacterList(session, m_service);
    m_logService.info("Operacao de personagem {} para {}: {}", success ? "aceita" : "rejeitada", session->getAccountName(), name);
}
