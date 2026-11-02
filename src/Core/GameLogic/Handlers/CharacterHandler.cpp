#include "GameLogic/Handlers/CharacterHandler.h"
#include "GameLogic/Services/CharacterService.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "Logging/LogService.h"
#include "GameLogic/GameWorld.h"
#include "Utils/SafeString.h"
#include <array>
#include <cstring>
#include <cmath>
CharacterHandler::CharacterHandler(CharacterService &service, GameWorld &world, LogService &logService) : m_service(service), m_world(world), m_logService(logService) {}

namespace
{
    const char *modelBodyForClass(int characterClass)
    {
        static constexpr std::array<const char *, 11> models{
            "char\\tmABCD\\b001.ini", "char\\tmABCD\\a001.ini", "char\\tmABCD\\d001.ini",
            "char\\tmABCD\\c001.ini", "char\\tmABCD\\ma001.ini", "char\\tmABCD\\mb001.ini",
            "char\\tmABCD\\mc001.ini", "char\\tmABCD\\md001.ini", "char\\tmABCD\\e001.ini",
            "char\\tmABCD\\me001.ini", "char\\tmABCD\\f001.ini"};
        return characterClass >= 1 && characterClass <= static_cast<int>(models.size())
            ? models[static_cast<size_t>(characterClass - 1)] : models[0];
    }

    const char *modelFaceForClass(int characterClass)
    {
        static constexpr std::array<const char *, 11> models{
            "char\\tmABCD\\tmh-b01.inf", "char\\tmABCD\\tmh-a01.inf", "char\\tmABCD\\tfh-D01.inf",
            "char\\tmABCD\\tmh-c01.inf", "char\\tmABCD\\Mmh-A01.inf", "char\\tmABCD\\Mfh-B01.inf",
            "char\\tmABCD\\Mfh-C01.inf", "char\\tmABCD\\Mmh-D01.inf", "char\\tmABCD\\tfh-e01.inf",
            "char\\tmABCD\\Mmh-e01.inf", "char\\tmABCD\\tfh-f01.inf"};
        return characterClass >= 1 && characterClass <= static_cast<int>(models.size())
            ? models[static_cast<size_t>(characterClass - 1)] : models[0];
    }

    uint32_t legacyJobCodeForClass(int characterClass)
    {
        static constexpr uint32_t codes[12] = {0, 1, 2, 3, 4, 6, 5, 8, 7, 9, 10, 11};
        return characterClass >= 1 && characterClass <= 11 ? codes[characterClass] : codes[1];
    }

    void sendCharacterList(const std::shared_ptr<ClientSession> &session, CharacterService &service)
    {
        PacketUserInfo response{};
        response.header.opcode = static_cast<uint32_t>(Opcodes::UserInfo);
        copyFixed(response.szUserID, session->getAccountName());
        const auto characters = service.getCharacterList(session->getAccountName());
        response.CharCount = static_cast<int32_t>(std::min<std::size_t>(characters.size(), 6));
        for (size_t i = 0; i < characters.size() && i < 6; ++i)
        {
            copyFixed(response.sCharacterData[i].szName, characters[i].szName);
            response.sCharacterData[i].iLevel = characters[i].iLevel;
            response.sCharacterData[i].JobCode = legacyJobCodeForClass(characters[i].iClass);
            copyFixed(response.sCharacterData[i].szModelName, modelBodyForClass(characters[i].iClass));
            copyFixed(response.sCharacterData[i].szModelName2, modelFaceForClass(characters[i].iClass));
            response.sCharacterData[i].StartField = characters[i].iMapId;
            response.sCharacterData[i].PosX = static_cast<int32_t>(characters[i].fPosX);
            response.sCharacterData[i].PosZ = static_cast<int32_t>(characters[i].fPosZ);
        }
        const auto *body = reinterpret_cast<const uint8_t *>(&response) + sizeof(PacketHeader);
        session->send(Packet(response.header.opcode, std::vector<uint8_t>(body, body + sizeof(response) - sizeof(PacketHeader))));
    }

    std::size_t sendNpcs(const std::shared_ptr<ClientSession>& session, const GameWorld& world, int mapId)
    {
        const auto npcs = world.npcsForMap(mapId);
        for (const auto& npc : npcs) {
            PacketNpcData data{};
            data.npcId = npc.id;
            data.objectSerial = npc.objectSerial;
            data.mapId = npc.mapId;
            data.x = static_cast<int32_t>(std::lround(npc.position.x));
            data.y = static_cast<int32_t>(std::lround(npc.position.y));
            data.z = static_cast<int32_t>(std::lround(npc.position.z));
            data.angleY = npc.angleY;
            data.eventCode = npc.eventCode;
            data.enabled = npc.enabled ? 1 : 0;
            copyFixed(data.szName, npc.name);
            copyFixed(data.szModelName, npc.modelName);
            const auto* bytes = reinterpret_cast<const uint8_t*>(&data);
            session->send(Packet(static_cast<uint32_t>(Opcodes::NpcData),
                                 std::vector<uint8_t>(bytes, bytes + sizeof(data))));
        }
        return npcs.size();
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
    std::optional<CharacterData> selectedCharacter;
    switch (static_cast<Opcodes>(packet.header.opcode))
    {
    case Opcodes::CreateCharacter:
        success = m_service.createCharacter(session->getAccountName(), name, command.LParam);
        break;
    case Opcodes::DeleteCharacter:
        success = m_service.deleteCharacter(session->getAccountName(), name);
        break;
    case Opcodes::SelectCharacter:
        selectedCharacter = m_service.selectCharacter(session->getAccountName(), name);
        success = selectedCharacter.has_value();
        if (success)
        {
            session->selectCharacter(name);
            m_world.enter(session->getAccountId(), session->getAccountName(), name, *selectedCharacter, session);
            m_logService.info("Personagem carregado para {}: HP {}/{}, MP {}/{}, STM {}/{}",
                              session->getAccountName(), selectedCharacter->iHP, selectedCharacter->iMaxHP,
                              selectedCharacter->iMP, selectedCharacter->iMaxMP,
                              selectedCharacter->iSTM, selectedCharacter->iMaxSTM);
        }
        break;
    default:
        return;
    }

    // The legacy client removes the character from its local list before
    // sending DELRECORDDATA; the original server sends no response here.
    if (static_cast<Opcodes>(packet.header.opcode) == Opcodes::DeleteCharacter)
    {
        m_logService.info("Exclusao de personagem {} para {}: {}", success ? "aceita" : "rejeitada",
                          session->getAccountName(), name);
        if (!success)
            sendCharacterList(session, m_service);
        return;
    }

    PacketRecordResult result{};
    result.header.opcode = static_cast<uint32_t>(success ? Opcodes::RecordResult : Opcodes::FailRecordData);
    result.Result = success ? 1 : 0;
    copyFixed(result.szName, name);
    const auto *body = reinterpret_cast<const uint8_t *>(&result) + sizeof(PacketHeader);
    std::vector<uint8_t> responseBody(body, body + sizeof(result) - sizeof(PacketHeader));
    if (success && static_cast<Opcodes>(packet.header.opcode) == Opcodes::SelectCharacter)
    {
        const auto *characterBytes = reinterpret_cast<const uint8_t *>(&*selectedCharacter);
        responseBody.insert(responseBody.end(), characterBytes, characterBytes + sizeof(CharacterData));
    }
    session->send(Packet(result.header.opcode, std::move(responseBody)));
    if (success && static_cast<Opcodes>(packet.header.opcode) == Opcodes::SelectCharacter)
    {
        const auto npcCount = sendNpcs(session, m_world, selectedCharacter->iMapId);
        m_logService.info("NPCs enviados para {}: mapa={}, quantidade={}", session->getAccountName(),
                          selectedCharacter->iMapId, npcCount);
    }
    if (success && static_cast<Opcodes>(packet.header.opcode) != Opcodes::SelectCharacter)
        sendCharacterList(session, m_service);
    m_logService.info("Operacao de personagem {} para {}: {} (classe {})", success ? "aceita" : "rejeitada",
                      session->getAccountName(), name, command.LParam);
}
