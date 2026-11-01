#include "Network/LegacyGatewayProtocol.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"

#include <algorithm>
#include <cstring>

namespace LegacyGateway
{

    namespace
    {

#pragma pack(push, 1)
        struct LegacyCharacterInfo
        {
            char name[32];
            char modelName[64];
            char modelName2[60];
            uint32_t modelNameCode2;
            uint32_t objectSerial;
            int32_t classClan, state, sizeLevel;
            uint32_t charSoundCode;
            uint32_t jobCode;
            int32_t level, strength, spirit, talent, dexterity, health;
            int32_t accuracy, attackRating, attackDamage[2], attackSpeed, shootingRange, criticalHit;
            int32_t defense, chanceBlock, absorption, moveSpeed, sight;
            int16_t weight[2], resistance[8], attackResistance[8], life[2], mana[2], stamina[2];
            float lifeRegen, manaRegen, staminaRegen;
            int32_t experience, nextExperience, money;
            uint32_t monsterInfo;
            uint32_t brood;
            int32_t statePoint;
            uint8_t updateInfo[4];
            int16_t arrowPosition[2];
            int32_t potionSpace, lifeFunction, manaFunction, staminaFunction;
            int16_t damageFunction[2];
            uint32_t reformCode, changeJob, jobBitMask;
            uint16_t playerKilling[2], playClass[2];
            int32_t experienceHigh;
            uint32_t eventTime;
            int16_t eventParam[2], presentItem[2], gravityScrollCheck[2];
            uint32_t temp[11], loginServerIp, loginServerSafeKey;
            uint16_t version[2];
        };

        struct LegacyRecordSkill
        {
            uint8_t skillPoint[16];
            uint16_t skillMastery[16];
            uint8_t shortKey[16];
            uint16_t selectedSkill[2];
        };

        struct LegacyQuestInfo
        {
            uint16_t questCode[2];
            uint32_t data[7];
        };

        struct LegacyLastQuestInfo
        {
            uint16_t lastQuest[32];
            int32_t lastQuestCount;
        };

        struct LegacyGameSaveInfo
        {
            uint32_t head;
            int32_t playStage, cameraMode, posX, posZ, lastMoney;
            uint32_t charInfoChecksum;
            LegacyRecordSkill skillInfo;
            int32_t saveTime, pcrNo;
            int16_t eventPlay[2], petInfo[2], potionUpdate[2], potionCount[3][4];
            uint16_t forceOrbUsing[2];
            uint32_t levelQuestLog;
            int32_t shortKeyNormalAttack, blessCastleTax;
            uint32_t blessCastleMaster, elementaryQuestLog;
            uint32_t primeItemTimes[12];
            int32_t totalSubExp, totalMoney, totalExp;
            char masterId[32];
            LegacyQuestInfo questInfo;
            LegacyLastQuestInfo lastQuestInfo;
            uint32_t serverId;
            uint16_t lifeBoosterUsing[2], manaBoosterUsing[2], staminaBoosterUsing[2], skillDelayUsing[2];
            uint32_t temp2[34];
        };

        struct LegacyRecordPrefix
        {
            int32_t size;
            int32_t code;
            char header[8];
            LegacyCharacterInfo character;
            LegacyGameSaveInfo save;
            uint32_t thrownItems[64][3];
            int32_t throwItemCount, itemCount, itemSubStart, dataSize;
        };

        struct LegacyPlayBuff
        {
            int32_t x, y, z;
            uint32_t frame;
            int16_t angle[4];
            int16_t stickItems[4];
            uint32_t targetSerial;
        };

        struct LegacyTransPlayData
        {
            int32_t size, code;
            uint32_t objectSerial, target;
            int32_t playBuffCount, startPosition;
            int16_t hp[2];
            uint32_t autoCharacterCode;
            int16_t area[2];
            uint8_t updateInfo[4], eventInfo[4];
        };

        struct LegacyTransPlayBuff
        {
            int32_t buffCount;
            LegacyPlayBuff playBuff;
        };

        struct LegacyAttackData
        {
            int32_t size, code;
            uint32_t destObjectSerial, targetObjectSerial;
            int32_t x, y, z;
            int32_t attackState, attackSize, power;
            uint32_t checkSum;
            int16_t rating[2];
            uint32_t damageCheckSum;
        };

        struct LegacyAttackData2
        {
            int32_t size, code;
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

        static_assert(sizeof(LegacyCharacterInfo) == 464);
        static_assert(sizeof(LegacyGameSaveInfo) == 512);
        static_assert(sizeof(LegacyPlayBuff) == 36);
        static_assert(sizeof(LegacyTransPlayData) == 44);
        static_assert(sizeof(LegacyTransPlayBuff) == 40);
        static_assert(sizeof(LegacyAttackData) == 52);
        static_assert(sizeof(LegacyAttackData2) == 92);

        uint32_t characterReformCode(const LegacyCharacterInfo &character)
        {
            uint32_t code = 0;
            uint32_t counter = 10;
            const auto add = [&code, &counter](const void *data, size_t size)
            {
                const auto *bytes = static_cast<const int8_t *>(data);
                for (size_t i = 0; i < size; ++i)
                    code += static_cast<uint32_t>(bytes[i]) * ((counter++ & 0x1FFFu) + 1u);
            };

            add(character.modelName2, sizeof(character.modelName2) + sizeof(character.modelNameCode2));
            add(&character.level, sizeof(character.level));
            add(&character.strength, sizeof(character.strength));
            add(&character.spirit, sizeof(character.spirit));
            add(&character.talent, sizeof(character.talent));
            add(&character.dexterity, sizeof(character.dexterity));
            add(&character.health, sizeof(character.health));
            add(&character.level, sizeof(character.level));
            add(&character.accuracy, sizeof(character.accuracy));
            add(&character.attackRating, sizeof(character.attackRating));
            add(character.attackDamage, sizeof(character.attackDamage));
            add(&character.attackSpeed, sizeof(character.attackSpeed));
            add(&character.shootingRange, sizeof(character.shootingRange));
            add(&character.criticalHit, sizeof(character.criticalHit));
            add(&character.defense, sizeof(character.defense));
            add(&character.chanceBlock, sizeof(character.chanceBlock));
            add(&character.absorption, sizeof(character.absorption));
            add(&character.moveSpeed, sizeof(character.moveSpeed));
            add(&character.sight, sizeof(character.sight));
            add(character.weight, sizeof(character.weight));
            add(character.resistance, sizeof(character.resistance));
            add(character.attackResistance, sizeof(character.attackResistance));
            add(character.life, sizeof(character.life));
            add(character.mana, sizeof(character.mana));
            add(character.stamina, sizeof(character.stamina));
            add(&character.experience, sizeof(character.experience));
            add(&character.nextExperience, sizeof(character.nextExperience));
            add(&character.money, sizeof(character.money));
            add(&character.statePoint, sizeof(character.statePoint));
            add(&character.experienceHigh, sizeof(character.experienceHigh));
            return code;
        }

        const char *modelBodyForClass(int characterClass)
        {
            static constexpr const char *models[] = {
                "char\\tmABCD\\b001.ini",  // 1: Fighter
                "char\\tmABCD\\a001.ini",  // 2: Mechanician
                "char\\tmABCD\\d001.ini",  // 3: Archer
                "char\\tmABCD\\c001.ini",  // 4: Pikeman
                "char\\tmABCD\\mb001.ini", // 5: Atalanta
                "char\\tmABCD\\ma001.ini", // 6: Knight
                "char\\tmABCD\\md001.ini", // 7: Magician
                "char\\tmABCD\\mc001.ini", // 8: Priestess
                "char\\tmABCD\\e001.ini",  // 9: Assassin
                "char\\tmABCD\\me001.ini", // 10: Shaman
                "char\\tmABCD\\f001.ini"   // 11: Martial Artist
            };

            // Atualizado para checar até a classe 11
            return characterClass >= 1 && characterClass <= 11 ? models[characterClass - 1] : models[0];
        }

        const char *modelFaceForClass(int characterClass)
        {
            static constexpr const char *models[] = {
                "char\\tmABCD\\tmh-b01.inf", // 1: Fighter
                "char\\tmABCD\\tmh-a01.inf", // 2: Mechanician
                "char\\tmABCD\\tfh-D01.inf", // 3: Archer
                "char\\tmABCD\\tmh-c01.inf", // 4: Pikeman
                "char\\tmABCD\\Mfh-B01.inf", // 5: Atalanta
                "char\\tmABCD\\Mmh-A01.inf", // 6: Knight
                "char\\tmABCD\\Mmh-D01.inf", // 7: Magician
                "char\\tmABCD\\Mfh-C01.inf", // 8: Priestess
                "char\\tmABCD\\tfh-e01.inf", // 9: Assassin
                "char\\tmABCD\\Mmh-e01.inf", // 10: Shaman
                "char\\tmABCD\\tfh-f01.inf"  // 11: Martial Artist (Artista Marcial)
            };

            // Atualizado para checar até a classe 11
            return characterClass >= 1 && characterClass <= 11 ? models[characterClass - 1] : models[0];
        }

        const char *modelHairForClass(int characterClass)
        {
            static constexpr const char *models[] = {
                "char\\tmABCD\\tmh-b01a.inf", // 1: Fighter (Cabelo padrão A)
                "char\\tmABCD\\tmh-a01a.inf", // 2: Mechanician (Cabelo padrão A)
                "char\\tmABCD\\tfh-D01a.inf", // 3: Archer (Cabelo padrão A)
                "char\\tmABCD\\tmh-c01a.inf", // 4: Pikeman (Cabelo padrão A)
                "char\\tmABCD\\Mfh-B01a.inf", // 5: Atalanta (Cabelo padrão A)
                "char\\tmABCD\\Mmh-A01a.inf", // 6: Knight (Cabelo padrão A)
                "char\\tmABCD\\Mmh-D01a.inf", // 7: Magician (Cabelo padrão A)
                "char\\tmABCD\\Mfh-C01a.inf", // 8: Priestess (Cabelo padrão A)
                "char\\tmABCD\\tfh-e01a.inf", // 9: Assassin (Cabelo padrão A)
                "char\\tmABCD\\Mmh-e01a.inf", // 10: Shaman (Cabelo padrão A)
                "char\\tmABCD\\tfh-f01a.inf"  // 11: Martial Artist (Cabelo padrão A)
            };

            return characterClass >= 1 && characterClass <= 11 ? models[characterClass - 1] : models[0];
        }

        std::vector<uint8_t> characterRecordPayload(const CharacterData &character)
        {
            LegacyRecordPrefix record{};
            record.code = static_cast<int32_t>(Opcode::RecordData);
            record.size = static_cast<int32_t>(sizeof(record));
            std::memcpy(record.header, "RC 1.50", 7);
            std::memcpy(record.character.name, character.szName, sizeof(record.character.name));
            strncpy_s(record.character.modelName, modelBodyForClass(character.iClass), _TRUNCATE);
            strncpy_s(record.character.modelName2, modelFaceForClass(character.iClass), _TRUNCATE);
            record.character.objectSerial = 0x10000000u + static_cast<uint32_t>(std::hash<std::string>{}(character.szName));
            record.character.state = 0x80; // smCHAR_STATE_USER
            record.character.brood = 0xA0; // smCHAR_MONSTER_USER
            record.character.jobCode = static_cast<uint32_t>(character.iClass);
            record.character.level = std::max(1, character.iLevel);
            record.character.strength = character.iStrength;
            record.character.spirit = character.iSpirit;
            record.character.talent = character.iTalent;
            record.character.dexterity = character.iAgility;
            record.character.health = character.iHealth;
            record.character.attackDamage[0] = character.iAttackMin;
            record.character.attackDamage[1] = character.iAttackMax;
            record.character.attackSpeed = 256;
            record.character.moveSpeed = 250;
            record.character.defense = character.iDefense;
            record.character.weight[1] = 100;
            record.character.life[0] = static_cast<int16_t>(std::clamp(character.iHP, 0, 32767));
            record.character.life[1] = static_cast<int16_t>(std::clamp(character.iMaxHP, 1, 32767));
            record.character.mana[0] = static_cast<int16_t>(std::clamp(character.iMP, 0, 32767));
            record.character.mana[1] = static_cast<int16_t>(std::clamp(character.iMaxMP, 1, 32767));
            record.character.stamina[0] = record.character.stamina[1] = 100;
            record.character.money = character.iGold;
            record.character.version[0] = 1;
            record.character.reformCode = characterReformCode(record.character);
            record.save.head = 150;
            record.save.playStage = character.iMapId;
            record.save.posX = static_cast<int32_t>(character.fPosX);
            record.save.posZ = static_cast<int32_t>(character.fPosZ);
            record.save.totalMoney = character.iGold;

            std::vector<uint8_t> frame(sizeof(record) + 32, 0);
            const int32_t frameSize = static_cast<int32_t>(frame.size());
            const int32_t total = 1;
            const int32_t dataSize = static_cast<int32_t>(sizeof(record));
            std::memcpy(frame.data(), &frameSize, sizeof(frameSize));
            std::memcpy(frame.data() + 4, &record.code, sizeof(record.code));
            std::memcpy(frame.data() + 12, &total, sizeof(total));
            std::memcpy(frame.data() + 16, &dataSize, sizeof(dataSize));
            std::memcpy(frame.data() + 20, &record, sizeof(record));
            return frame;
        }

    } // namespace

    bool isSafeString(const char *value, size_t capacity)
    {
        return value != nullptr && std::memchr(value, '\0', capacity) != nullptr;
    }

    std::optional<Packet> toModernPacket(const std::vector<uint8_t> &payload)
    {
        if (payload.size() < sizeof(Command) || payload.size() > Constants::MAX_PKTSIZ)
            return std::nullopt;

        Command command{};
        std::memcpy(&command, payload.data(), std::min(payload.size(), sizeof(command)));
        if (command.size < static_cast<int32_t>(sizeof(int32_t) * 2) ||
            command.size > static_cast<int32_t>(payload.size()))
            return std::nullopt;

        // Only explicitly supported login-stage commands are translated. Unknown
        // legacy opcodes are rejected instead of being guessed.
        uint32_t modernOpcode = 0;
        switch (static_cast<Opcode>(command.code))
        {
        case Opcode::Connected:
            modernOpcode = static_cast<uint32_t>(Opcodes::KeepAlive);
            break;
        case Opcode::RecordData:
        {
            // TRANS_RECORD_DATAS: five integers, serialized record and 12 legacy padding bytes.
            if (command.size != static_cast<int32_t>(payload.size()) || command.size < 32 ||
                command.lParam < 0 || command.wParam <= 0 || command.wParam > 64 ||
                command.lParam >= command.wParam || command.sParam <= 0 ||
                command.sParam > command.size - 32)
                return std::nullopt;
            modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
            break;
        }
        case Opcode::SetBlacklist:
            if (command.size != sizeof(Command) || payload.size() != sizeof(Command))
                return std::nullopt;
            // The legacy client reports local protection status through this command.
            // Do not trust or execute its values; only acknowledge its fixed layout.
            modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
            break;
        case Opcode::PlayerInfo:
            // smTRNAS_PLAYERINFO uses the 32-bit legacy smCHAR_INFO layout: 504 bytes.
            if (command.size != 504 || payload.size() != 504)
                return std::nullopt;
            modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
            break;
        case Opcode::TransPlayData:
        {
            // smTRANS_PLAYDATA (44) + smTRANS_PLAYBUFF (40 each) + smEFFECT_ITEM (32 each).
            // The effect count is carried in bEventInfo[TRANS_PLAYDATA_EVENT_EXT].
            if (command.size != static_cast<int32_t>(payload.size()) || payload.size() < 44 || payload.size() > 4096)
                return std::nullopt;
            int32_t playBuffCount = 0;
            {
                std::memcpy(&playBuffCount, payload.data() + 16, sizeof(playBuffCount));
                const uint8_t effectCount = payload[40 + 3];
                const size_t expectedSize = 44u + static_cast<size_t>(playBuffCount) * 40u +
                                            static_cast<size_t>(effectCount) * 32u;
                if (playBuffCount <= 0 || playBuffCount > 64 || effectCount > 32 || expectedSize != payload.size())
                    return std::nullopt;
            }
            if (playBuffCount <= 0)
                return std::nullopt;
            const size_t playBuffOffset = sizeof(LegacyTransPlayData);
            if (payload.size() < playBuffOffset + sizeof(LegacyTransPlayBuff))
                return std::nullopt;
            LegacyTransPlayBuff playBuff{};
            std::memcpy(&playBuff, payload.data() + playBuffOffset, sizeof(playBuff));
            if (playBuff.buffCount < 0 || playBuff.buffCount > 64)
                return std::nullopt;
            std::vector<uint8_t> movement(sizeof(uint32_t) + sizeof(int32_t) * 3);
            std::memcpy(movement.data(), payload.data() + 8, sizeof(uint32_t));
            std::memcpy(movement.data() + 4, &playBuff.playBuff.x, sizeof(int32_t) * 3);
            return Packet(static_cast<uint32_t>(Opcodes::PlayData), movement);
        }
        case Opcode::ProcessInfo:
            // TRANS_CHATMESSAGE carries two DWORDs followed by a bounded process path.
            if (command.size != static_cast<int32_t>(payload.size()) || command.size < 17 ||
                command.size > 512 || !isSafeString(reinterpret_cast<const char *>(payload.data() + 16), payload.size() - 16))
                return std::nullopt;
            modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
            break;
        case Opcode::ChatMessage:
            // TRANS_CHATMESSAGE: size, code, IP, object serial and 256-byte message.
            if (command.size != 272 || payload.size() != 272 ||
                !isSafeString(reinterpret_cast<const char *>(payload.data() + 16), 256))
                return std::nullopt;
            {
                std::vector<uint8_t> body(sizeof(uint32_t) + 256);
                std::memcpy(body.data(), payload.data() + 12, sizeof(uint32_t));
                std::memcpy(body.data() + sizeof(uint32_t), payload.data() + 16, 256);
                return Packet(static_cast<uint32_t>(Opcodes::ChatMessage), body);
            }
        case Opcode::GetUserInfo:
            return loginToModernPacket(payload);
        case Opcode::CheckIP:
            if (command.size != 52 || command.wParam < 0 || command.wParam > 3 ||
                !isSafeString(reinterpret_cast<const char *>(payload.data() + 20), 32))
                return std::nullopt;
            return Packet(static_cast<uint32_t>(Opcodes::KeepAlive));
        case Opcode::FunctionResult:
            // The legacy client sends this fixed command for its client-function
            // check. Validate the frame and continue; the modern server has no
            // executable-memory probe to run on the client.
            if (command.size != sizeof(Command) || payload.size() < sizeof(Command))
                return std::nullopt;
            return Packet(static_cast<uint32_t>(Opcodes::KeepAlive));
        case Opcode::EffectLowPet:
            // Legacy movement may carry a low-level-pet effect command. The
            // modern server has no pet state yet, but the bounded command must
            // not tear down an otherwise authenticated game session.
            if (command.size != static_cast<int32_t>(payload.size()) || command.size < static_cast<int32_t>(sizeof(Command)) ||
                command.size > 512)
                return std::nullopt;
            modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
            break;
        default:
            if (command.code == static_cast<int32_t>(Opcode::AttackData) &&
                command.size == static_cast<int32_t>(sizeof(LegacyAttackData)) &&
                payload.size() >= sizeof(LegacyAttackData))
            {
                LegacyAttackData attack{};
                std::memcpy(&attack, payload.data(), sizeof(attack));
                if (attack.destObjectSerial == 0 || attack.targetObjectSerial == 0 || attack.attackSize < 0)
                    return std::nullopt;
                std::vector<uint8_t> body(sizeof(uint32_t) * 2);
                std::memcpy(body.data(), &attack.destObjectSerial, sizeof(uint32_t));
                std::memcpy(body.data() + sizeof(uint32_t), &attack.targetObjectSerial, sizeof(uint32_t));
                return Packet(static_cast<uint32_t>(Opcodes::AttackData), body);
            }
            if (command.code == static_cast<int32_t>(Opcode::AttackData2) &&
                command.size == static_cast<int32_t>(sizeof(LegacyAttackData2)) &&
                payload.size() >= sizeof(LegacyAttackData2))
            {
                LegacyAttackData2 attack{};
                std::memcpy(&attack, payload.data(), sizeof(attack));
                if (attack.destObjectSerial == 0 || attack.targetObjectSerial == 0 || attack.attackSize < 0)
                    return std::nullopt;
                std::vector<uint8_t> body(sizeof(uint32_t) * 2);
                std::memcpy(body.data(), &attack.destObjectSerial, sizeof(uint32_t));
                std::memcpy(body.data() + sizeof(uint32_t), &attack.targetObjectSerial, sizeof(uint32_t));
                return Packet(static_cast<uint32_t>(Opcodes::AttackData2), body);
            }
            // The legacy client uses GETRECORDDATA (opcode 5) on the login/data
            // socket when it requests the selected character after choosing a
            // server. Translate only its fixed, bounded command layout.
            if ((command.code == 5 && command.size == 52 && payload.size() >= 52) ||
                ((command.code == 10 || command.code == 11) && command.size == 76 && payload.size() >= 76))
            {
                PacketCharacterCommand modern{};
                modern.header.opcode = static_cast<uint32_t>(
                    command.code == 5 ? Opcodes::SelectCharacter : command.code == 10 ? Opcodes::DeleteCharacter
                                                                                      : Opcodes::CreateCharacter);
                std::memcpy(&modern.LParam, payload.data() + 8, sizeof(modern.LParam));
                std::memcpy(&modern.WParam, payload.data() + 12, sizeof(modern.WParam));
                std::memcpy(&modern.SParam, payload.data() + 16, sizeof(modern.SParam));
                if (command.code == 11)
                {
                    // Older clients leave LParam at zero. Keep that client
                    // compatible, but never accept a class outside the game
                    // range when an updated client supplies it.
                    if (modern.LParam == 0)
                        modern.LParam = 1;
                    else if (modern.LParam < 1 || modern.LParam > 11)
                        return std::nullopt;
                }
                if (command.code != 5)
                    std::memcpy(modern.szAccount, payload.data() + 20, 21);
                const auto *name = reinterpret_cast<const char *>(payload.data() + (command.code == 5 ? 20 : 41));
                std::memcpy(modern.szName, name, sizeof(modern.szName));
                const auto *body = reinterpret_cast<const uint8_t *>(&modern) + sizeof(PacketHeader);
                return Packet(modern.header.opcode,
                              std::vector<uint8_t>(body, body + sizeof(modern) - sizeof(PacketHeader)));
            }
            return std::nullopt;
        }

        // Nunca construa um range de iteradores diretamente a partir de um
        // tamanho recebido da rede. O Debug STL aborta se esse valor passar
        // de end(), mesmo antes de qualquer acesso ao conteúdo.
        if (command.size < static_cast<int32_t>(sizeof(Command)) ||
            command.size > static_cast<int32_t>(payload.size()))
            return std::nullopt;
        const size_t bodySize = static_cast<size_t>(command.size) - sizeof(Command);
        std::vector<uint8_t> body(bodySize);
        if (bodySize != 0)
            std::memcpy(body.data(), payload.data() + sizeof(Command), bodySize);
        return Packet(modernOpcode, body);
    }

    std::optional<Packet> loginToModernPacket(const std::vector<uint8_t> &payload)
    {
        if (payload.size() != sizeof(LoginPacket))
            return std::nullopt;

        LoginPacket legacy{};
        std::memcpy(&legacy, payload.data(), sizeof(legacy));
        if (legacy.size != static_cast<int32_t>(sizeof(LoginPacket)) ||
            legacy.code != static_cast<int32_t>(Opcode::GetUserInfo) ||
            !isSafeString(legacy.account, sizeof(legacy.account)) ||
            !isSafeString(legacy.password, sizeof(legacy.password)) ||
            !isSafeString(legacy.hash, sizeof(legacy.hash)) ||
            !isSafeString(legacy.mac, sizeof(legacy.mac)))
        {
            return std::nullopt;
        }

        PacketLoginUser modern{};
        modern.header.opcode = static_cast<uint32_t>(Opcodes::LoginUser);
        std::memcpy(modern.szUserID, legacy.account, sizeof(legacy.account));
        std::memcpy(modern.szPassword, legacy.password, sizeof(legacy.password));
        std::memcpy(modern.szMacAddr, legacy.mac, sizeof(legacy.mac));
        std::memcpy(modern.szHardwareID, legacy.hash, sizeof(legacy.hash));
        modern.uWidthScreen = 800;
        modern.uHeightScreen = 600;
        modern.iVersion = 1610;

        const auto *body = reinterpret_cast<const uint8_t *>(&modern) + sizeof(PacketHeader);
        return Packet(modern.header.opcode,
                      std::vector<uint8_t>(body, body + sizeof(modern) - sizeof(PacketHeader)));
    }

    std::optional<std::vector<uint8_t>> toLegacyPayload(const Packet &packet, uint32_t packetKey, int32_t requestCode)
    {
        std::vector<uint8_t> payload;
        auto append = [&payload](const void *data, size_t size)
        {
            const auto *p = static_cast<const uint8_t *>(data);
            payload.insert(payload.end(), p, p + size);
        };

        if (packet.header.opcode == static_cast<uint32_t>(Opcodes::LegacyConnected))
        {
            Command command{sizeof(Command), static_cast<int32_t>(Opcode::Connected), 0, 0, 0, 0};
            append(&command, sizeof(command));
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::KeepAlive))
        {
            Command command{sizeof(Command), static_cast<int32_t>(Opcode::Connected), 0, 0, 0, 0};
            append(&command, sizeof(command));
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::AccountLoginCode))
        {
            // PacketAccountLoginCode body: dwReserved, iCode, iFailCode, message.
            // O primeiro DWORD não é o resultado; ler esse campo fazia toda falha
            // de senha parecer sucesso para o cliente legado.
            if (packet.body.size() < sizeof(uint32_t) + sizeof(int32_t))
                return std::nullopt;
            int32_t result = 0;
            std::memcpy(&result, packet.body.data() + sizeof(uint32_t), sizeof(result));
            if (result == 1) // LoginResult::Success
                return std::vector<uint8_t>{}; // Success is followed by server/character lists.
            Command command{sizeof(Command), static_cast<int32_t>(Opcode::FailConnect), 0, result, 0, 1};
            append(&command, sizeof(command));
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::ServerList))
        {
            constexpr size_t serverListPrefix = offsetof(PacketServerList, sServers) - sizeof(PacketHeader);
            if (packet.body.size() < serverListPrefix)
                return std::nullopt;
            PacketServerList modern{};
            const size_t copySize = std::min(packet.body.size(), sizeof(modern) - sizeof(PacketHeader));
            std::memcpy(reinterpret_cast<uint8_t *>(&modern) + sizeof(PacketHeader), packet.body.data(), copySize);
            struct LegacyServerInfo
            {
                char name[32], ip1[20], ip2[20], ip3[20];
                uint32_t port1, port2, port3, state;
            };
            struct LegacyServerList
            {
                int32_t size, code;
                char name[16];
                uint32_t time;
                int32_t clanTicket, temp, clanCount, count;
                LegacyServerInfo servers[8];
            } legacy{};
            legacy.size = sizeof(legacy);
            legacy.code = static_cast<int32_t>(Opcode::ServerInfo);
            std::memcpy(legacy.name, modern.szServerName, sizeof(legacy.name));
            legacy.clanCount = 0;
            legacy.count = std::min(modern.iGameServers, 8);
            for (int i = 0; i < legacy.count; ++i)
            {
                std::memcpy(legacy.servers[i].name, modern.sServers[i].szName, sizeof(legacy.servers[i].name));
                std::memcpy(legacy.servers[i].ip1, modern.sServers[i].szaIP[0], sizeof(legacy.servers[i].ip1));
                std::memcpy(legacy.servers[i].ip2, modern.sServers[i].szaIP[1], sizeof(legacy.servers[i].ip2));
                std::memcpy(legacy.servers[i].ip3, modern.sServers[i].szaIP[2], sizeof(legacy.servers[i].ip3));
                legacy.servers[i].port1 = modern.sServers[i].iaPort[0];
                legacy.servers[i].port2 = modern.sServers[i].iaPort[1];
                legacy.servers[i].port3 = modern.sServers[i].iaPort[2];
            }
            append(&legacy, sizeof(legacy));
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::UserInfo))
        {
            if (packet.body.size() != sizeof(PacketUserInfo) - sizeof(PacketHeader))
                return std::nullopt;
            PacketUserInfo modern{};
            std::memcpy(reinterpret_cast<uint8_t *>(&modern) + sizeof(PacketHeader), packet.body.data(), packet.body.size());
            struct LegacyCharacter
            {
                char name[32], model[64], model2[64];
                uint32_t job, level, brood, armor;
                int start, x, z;
                uint32_t temp[13];
            };
            struct LegacyUsers
            {
                int32_t size, code;
                char id[21];
                int32_t count;
                LegacyCharacter chars[6];
            } legacy{};
            legacy.size = sizeof(legacy);
            legacy.code = static_cast<int32_t>(Opcode::SetUserInfo);
            std::memcpy(legacy.id, modern.szUserID, sizeof(legacy.id));
            legacy.count = std::clamp(modern.CharCount, 0, 6);
            for (int i = 0; i < legacy.count; ++i)
            {
                const auto &src = modern.sCharacterData[i];
                auto &dst = legacy.chars[i];
                std::memcpy(dst.name, src.szName, sizeof(dst.name));
                std::memcpy(dst.model, src.szModelName, sizeof(dst.model));
                std::memcpy(dst.model2, src.szModelName2, sizeof(dst.model2));
                dst.job = src.JobCode;
                dst.level = src.iLevel;
                dst.brood = src.Brood;
                dst.armor = src.dwArmorCode;
                dst.start = src.StartField;
                dst.x = src.PosX;
                dst.z = src.PosZ;
                std::memcpy(dst.temp, src.dwTemp, sizeof(dst.temp));
            }
            append(&legacy, sizeof(legacy));
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::RecordResult) ||
                 packet.header.opcode == static_cast<uint32_t>(Opcodes::FailRecordData))
        {
            if (packet.body.size() < sizeof(PacketRecordResult) - sizeof(PacketHeader))
                return std::nullopt;
            PacketRecordResult result{};
            std::memcpy(reinterpret_cast<uint8_t *>(&result) + sizeof(PacketHeader), packet.body.data(),
                        sizeof(result) - sizeof(PacketHeader));
            if (packet.header.opcode == static_cast<uint32_t>(Opcodes::RecordResult) && requestCode == 5 &&
                result.Result != 0 && packet.body.size() >= sizeof(PacketRecordResult) - sizeof(PacketHeader) + sizeof(CharacterData))
            {
                CharacterData character{};
                std::memcpy(&character, packet.body.data() + sizeof(PacketRecordResult) - sizeof(PacketHeader), sizeof(character));
                return characterRecordPayload(character);
            }
            const int32_t responseCode = requestCode == 10 || requestCode == 11
                                             ? requestCode
                                             : static_cast<int32_t>(packet.header.opcode == static_cast<uint32_t>(Opcodes::RecordResult) ? 7 : 6);
            Command legacy{sizeof(Command), responseCode,
                           0, result.Result, 0, 0};
            append(&legacy, sizeof(legacy));
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::AttackResult) ||
                 packet.header.opcode == static_cast<uint32_t>(Opcodes::LastAttackDamage))
        {
            if (packet.body.size() != sizeof(int32_t))
                return std::nullopt;
            int32_t value = 0;
            std::memcpy(&value, packet.body.data(), sizeof(value));
            const auto legacyCode = packet.header.opcode == static_cast<uint32_t>(Opcodes::AttackResult)
                                        ? Opcode::AttackResult
                                        : Opcode::DispDamage;
            Command legacy{sizeof(Command), static_cast<int32_t>(legacyCode), 0, value, 0, 0};
            append(&legacy, sizeof(legacy));
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::ChatMessage))
        {
            if (packet.body.size() != sizeof(uint32_t) + 256 ||
                !isSafeString(reinterpret_cast<const char *>(packet.body.data() + sizeof(uint32_t)), 256))
                return std::nullopt;
            struct LegacyChatMessage {
                int32_t size, code;
                uint32_t ip, objectSerial;
                char message[256];
            } legacy{};
            legacy.size = sizeof(legacy);
            legacy.code = static_cast<int32_t>(Opcode::ChatMessageLegacy);
            std::memcpy(&legacy.objectSerial, packet.body.data(), sizeof(legacy.objectSerial));
            std::memcpy(legacy.message, packet.body.data() + sizeof(uint32_t), sizeof(legacy.message));
            append(&legacy, sizeof(legacy));
        }
        else
        {
            return std::nullopt;
        }
        if (payload.empty())
            return payload;
        return payload;
    }

    std::vector<uint8_t> gameServerReadyPayload()
    {
        // The original SrcServer sends VERSION immediately after a GameServer
        // connection. SERVER_INFO belongs to the login/server-selection stage.
        constexpr int32_t legacyClientVersion = 4135;
        Command legacy{sizeof(Command), static_cast<int32_t>(Opcode::Version), 0, legacyClientVersion, 0, 0};
        const auto *bytes = reinterpret_cast<const uint8_t *>(&legacy);
        return std::vector<uint8_t>(bytes, bytes + sizeof(legacy));
    }

} // namespace LegacyGateway
