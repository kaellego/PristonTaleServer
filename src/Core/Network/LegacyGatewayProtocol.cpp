#include "Network/LegacyGatewayProtocol.h"
#include "Network/LegacyPacketCodec.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "Utils/SafeString.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <limits>

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
            char masterId[21];
            uint8_t masterIdPadding[3];
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

        struct LegacyPlayData
        {
            int32_t size, code;
            uint32_t objectSerial;
            int32_t x, y, z;
            int16_t angle[4];
            uint32_t frame;
            uint32_t autoCharacterCode;
        };

        struct LegacyPlayDataGroup
        {
            int32_t size, code;
            int32_t playDataCount;
            uint32_t sendCode;
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

        struct LegacyPlayPosition
        {
            int32_t size, code;
            int32_t area, x, y, z;
            int16_t hp[2];
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

        // Layouts from smPacket.h. The target array is variable-length on the
        // wire; only the advertised prefix plus TargetCount serials is sent.
        struct LegacySkillAttackData
        {
            int32_t size, code;
            uint32_t destObjectSerial;
            int32_t x, y, z;
            int32_t attackState, attackSize, power;
            uint32_t checkSum;
            int32_t targetCount;
            uint32_t targetSerials[42];
        };

        struct LegacySkillAttackData2
        {
            int32_t size, code;
            uint32_t destObjectSerial;
            uint32_t checkSum;
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
            uint32_t mainTargetObject;
            int32_t targetCount;
            uint32_t targetSerials[42];
        };

        // x86 wire representation of the legacy sITEM. The two DirectDraw
        // members are handles/pointers in the original process and are never
        // dereferenced by this server.
        struct LegacyItemWire32
        {
            uint32_t code;
            uint8_t itemNameIndex[32];
            uint8_t lastCategory[16];
            int32_t width, height;
            uint8_t itemFilePath[64];
            uint32_t itemClass;
            uint8_t dropItem[64];
            uint32_t modelPosition;
            int32_t soundIndex, weaponClass, flags, x, y, setX, setY;
            uint32_t lpItem;
            int32_t itemPosition, potionCount, notUseFlag, sellPrice, oldX, oldY;
            uint32_t lpTempItem;
            uint8_t itemInfo[532];
        };

        struct LegacyBuyShopItem
        {
            int32_t size, code;
            LegacyItemWire32 item;
            int32_t itemCount;
        };

        struct LegacySellShopItem
        {
            int32_t size, code;
            uint8_t itemInfo[532];
            int32_t x, y, z;
            uint32_t securityCode[4];
        };

        // TRANS_USINGITEM / OPCODE_INVEN_POSITION2. The client reports item
        // positions as control traffic; these values never mutate inventory.
        struct LegacyInventoryPosition
        {
            int32_t size, code;
            int32_t itemCount;
            struct Entry
            {
                uint32_t itemCode;
                int16_t performance[8];
            } items[16];
        };

        struct LegacyNpcInfo
        {
            int32_t size, code;
            LegacyCharacterInfo character;
            uint32_t objectSerial;
            int32_t x, y, z;
            int32_t ax, ay, az;
            int32_t state;
        };
#pragma pack(pop)

        static_assert(sizeof(LegacyCharacterInfo) == 464);
        static_assert(sizeof(LegacyGameSaveInfo) == 504);
        static_assert(sizeof(LegacyRecordPrefix) == 1768);
        static_assert(sizeof(LegacyPlayBuff) == 36);
        static_assert(sizeof(LegacyPlayData) == 40);
        static_assert(sizeof(LegacyPlayDataGroup) == 16);
        static_assert(sizeof(LegacyTransPlayData) == 44);
        static_assert(sizeof(LegacyPlayPosition) == 28);
        static_assert(sizeof(LegacyTransPlayBuff) == 40);
        static_assert(sizeof(LegacyAttackData) == 52);
        static_assert(sizeof(LegacyAttackData2) == 92);
        static_assert(sizeof(LegacySkillAttackData) == 212);
        static_assert(sizeof(LegacySkillAttackData2) == 264);
        static_assert(sizeof(LegacyItemWire32) == 788);
        static_assert(sizeof(LegacyBuyShopItem) == 800);
        static_assert(sizeof(LegacySellShopItem) == 568);
        static_assert(sizeof(LegacyInventoryPosition) == 332);
        static_assert(sizeof(LegacyNpcInfo) == 504);

        constexpr size_t LegacyItemInfoSize = 532;

        template <typename T>
        void writeLegacyItemField(std::array<uint8_t, LegacyItemInfoSize>& item, size_t offset, T value)
        {
            if (offset + sizeof(T) <= item.size())
                std::memcpy(item.data() + offset, &value, sizeof(T));
        }

        std::vector<uint8_t> encodeLegacyShopItem(const PacketNpcShopListEntry& source, int itemKind)
        {
            std::array<uint8_t, LegacyItemInfoSize> item{};
            const int32_t itemSize = static_cast<int32_t>(LegacyItemInfoSize);
            const int32_t price = static_cast<int32_t>(std::clamp<int64_t>(source.buyPrice, 0, std::numeric_limits<int32_t>::max()));
            const int32_t level = std::clamp(source.requiredLevel, 1, 2000);
            const int16_t durability = 100;
            const uint32_t allJobs = 0xFFFFFFFFu;
            const uint32_t kind = static_cast<uint32_t>(itemKind);

            // Offsets are the x86 sITEMINFO layout from sinItem.h. No pointer
            // from the client is copied into this definition.
            writeLegacyItemField(item, 0, itemSize);
            writeLegacyItemField(item, 20, durability);
            writeLegacyItemField(item, 22, durability);
            writeLegacyItemField(item, 24, static_cast<uint32_t>(source.itemCode));
            std::memcpy(item.data() + 28, source.itemName, std::min<size_t>(32, sizeof(source.itemName)));
            writeLegacyItemField(item, 64, price);
            writeLegacyItemField(item, 72, 1);
            writeLegacyItemField(item, 168, level);
            writeLegacyItemField(item, 200, durability);
            writeLegacyItemField(item, 204, price);
            writeLegacyItemField(item, 244, allJobs);
            writeLegacyItemField(item, 468, kind);

            std::vector<uint8_t> compressed(LegacyItemInfoSize * 2 + 16, 0);
            size_t sourceOffset = 0;
            size_t destinationOffset = sizeof(uint32_t);
            while (sourceOffset < item.size() && destinationOffset < compressed.size()) {
                const uint8_t first = item[sourceOffset];
                const uint8_t second = sourceOffset + 1 < item.size() ? item[sourceOffset + 1] : first;
                if (first == 0 && second == 0) {
                    size_t count = 0;
                    while (sourceOffset < item.size() && item[sourceOffset] == 0 && count < 0x7F) {
                        ++sourceOffset;
                        ++count;
                    }
                    compressed[destinationOffset++] = static_cast<uint8_t>(count | 0x80);
                } else {
                    const size_t marker = destinationOffset++;
                    size_t count = 0;
                    while (sourceOffset < item.size() && destinationOffset < compressed.size() && count < 0x7F) {
                        const uint8_t current = item[sourceOffset];
                        const uint8_t next = sourceOffset + 1 < item.size() ? item[sourceOffset + 1] : current;
                        if (current == 0 && next == 0)
                            break;
                        compressed[destinationOffset++] = current;
                        ++sourceOffset;
                        ++count;
                    }
                    compressed[marker] = static_cast<uint8_t>(count);
                }
            }
            const int32_t compressedSize = static_cast<int32_t>(destinationOffset);
            std::memcpy(compressed.data(), &compressedSize, sizeof(compressedSize));
            compressed.resize(destinationOffset);
            return compressed;
        }

        uint32_t legacyAttackChecksum(const LegacyAttackData2& attack)
        {
            uint32_t checksum = static_cast<uint32_t>(attack.code);
            checksum += static_cast<uint32_t>(attack.x ^ attack.y ^ attack.z);
            checksum += static_cast<uint32_t>(attack.attackCount) ^ attack.time;

            uint32_t value = 0;
            std::memcpy(&value, attack.power, sizeof(value));
            checksum ^= value;
            std::memcpy(&value, attack.power2, sizeof(value));
            checksum ^= value;
            std::memcpy(&value, attack.critical, sizeof(value));
            checksum ^= value;
            std::memcpy(&value, attack.motionCount, sizeof(value));
            checksum ^= value;
            checksum += static_cast<uint32_t>(attack.skillCode);
            return checksum ^ 0x68098051u;
        }

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
                "char\\tmABCD\\ma001.ini", // 5: Knight
                "char\\tmABCD\\mb001.ini", // 6: Atalanta
                "char\\tmABCD\\mc001.ini", // 7: Priestess
                "char\\tmABCD\\md001.ini", // 8: Magician
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
                "char\\tmABCD\\Mmh-A01.inf", // 5: Knight
                "char\\tmABCD\\Mfh-B01.inf", // 6: Atalanta
                "char\\tmABCD\\Mfh-C01.inf", // 7: Priestess
                "char\\tmABCD\\Mmh-D01.inf", // 8: Magician
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

        // CharacterData stores the project's class order. The original
        // client expects the legacy display/job code, not JobBitCode.
        uint32_t legacyJobCodeForClass(int characterClass)
        {
            static constexpr uint32_t codes[] = {
                0,
                1u,  // Fighter
                2u,  // Mechanician
                3u,  // Archer
                4u,  // Pikeman
                6u,  // Knight
                5u,  // Atalanta
                8u,  // Priestess
                7u,  // Magician
                9u,  // Assassin (extended clients)
                10u, // Shaman (extended clients)
                11u  // Martial Artist (extended clients)
            };
            return characterClass >= 1 && characterClass <= 11
                       ? codes[static_cast<size_t>(characterClass)]
                       : codes[1];
        }

        struct LegacyClassRules
        {
            int sight;
            int lifeFunction;
            int manaFunction;
            int staminaFunction;
            int damageFunction[2];
        };

        LegacyClassRules legacyClassRulesForClass(int characterClass)
        {
            static constexpr LegacyClassRules rules[12]{
                {},
                {70, 1, 3, 1, {1, 2}},  // Fighter
                {65, 2, 2, 2, {2, 2}},  // Mechanician
                {80, 3, 3, 2, {3, 1}},  // Archer
                {75, 1, 3, 1, {1, 2}},  // Pikeman
                {190, 2, 2, 2, {1, 2}}, // Knight
                {200, 2, 2, 2, {2, 1}}, // Atalanta
                {200, 4, 1, 3, {3, 2}}, // Priestess
                {190, 5, 1, 3, {3, 2}}, // Magician
                {80, 3, 3, 2, {3, 1}},  // Assassin
                {190, 5, 1, 3, {3, 2}}, // Shaman
                {70, 1, 3, 1, {1, 2}}   // Martial Artist
            };
            return rules[characterClass >= 1 && characterClass <= 11 ? characterClass : 1];
        }

        int internalClassForLegacyJobCode(int legacyJobCode)
        {
            switch (legacyJobCode)
            {
            case 1:
                return 1; // Fighter
            case 2:
                return 2; // Mechanician
            case 3:
                return 3; // Archer
            case 4:
                return 4; // Pikeman
            case 5:
                return 6; // Atalanta
            case 6:
                return 5; // Knight
            case 7:
                return 8; // Magician
            case 8:
                return 7; // Priestess
            case 9:
                return 9; // Assassin
            case 10:
                return 10; // Shaman
            case 11:
                return 11; // Martial Artist
            default:
                return 0;
            }
        }

        bool validLegacyCharacterName(const char *value, size_t capacity)
        {
            if (!value || capacity == 0 || std::memchr(value, '\0', capacity) == nullptr)
                return false;
            const auto length = strnlen_s(value, capacity);
            if (length == 0 || length >= 32)
                return false;
            for (size_t i = 0; i < length; ++i)
            {
                const auto ch = static_cast<unsigned char>(value[i]);
                if (!std::isalnum(ch) && ch != '_' && ch != '-')
                    return false;
            }
            return true;
        }

        bool copyLegacyCharacterName(char *destination, size_t destinationSize,
                                     const std::vector<uint8_t> &payload,
                                     size_t sourceOffset)
        {
            if (!destination || destinationSize == 0 || sourceOffset + 32 > payload.size())
                return false;
            const auto *source = reinterpret_cast<const char *>(payload.data() + sourceOffset);
            if (!validLegacyCharacterName(source, 32))
                return false;
            std::memset(destination, 0, destinationSize);
            std::memcpy(destination, source, std::min(destinationSize - 1, std::strlen(source)));
            return true;
        }

        std::vector<uint8_t> characterRecordPayload(const CharacterData &character)
        {
            LegacyRecordPrefix record{};
            record.code = static_cast<int32_t>(Opcode::RecordData);
            record.size = static_cast<int32_t>(sizeof(record));
            std::memcpy(record.header, "RC 1.50", 7);
            std::memcpy(record.character.name, character.szName, sizeof(record.character.name));
            copyFixed(record.character.modelName, modelBodyForClass(character.iClass));
            copyFixed(record.character.modelName2, modelFaceForClass(character.iClass));
            record.character.objectSerial = 0x10000000u + static_cast<uint32_t>(std::hash<std::string>{}(character.szName));
            record.character.state = 0x80; // smCHAR_STATE_USER
            record.character.brood = 0xA0; // smCHAR_MONSTER_USER
            record.character.jobCode = legacyJobCodeForClass(character.iClass);
            const auto classRules = legacyClassRulesForClass(character.iClass);
            record.character.sight = classRules.sight;
            record.character.lifeFunction = classRules.lifeFunction;
            record.character.manaFunction = classRules.manaFunction;
            record.character.staminaFunction = classRules.staminaFunction;
            record.character.damageFunction[0] = static_cast<int16_t>(classRules.damageFunction[0]);
            record.character.damageFunction[1] = static_cast<int16_t>(classRules.damageFunction[1]);
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
            const int hpMax = std::max(100, character.iMaxHP);
            const int mpMax = std::max(100, character.iMaxMP);
            record.character.life[0] = static_cast<int16_t>(std::clamp(character.iHP > 0 ? character.iHP : hpMax, 1, 32767));
            record.character.life[1] = static_cast<int16_t>(std::clamp(hpMax, 1, 32767));
            record.character.mana[0] = static_cast<int16_t>(std::clamp(character.iMP > 0 ? character.iMP : mpMax, 1, 32767));
            record.character.mana[1] = static_cast<int16_t>(std::clamp(mpMax, 1, 32767));
            const int stmMax = std::max(100, character.iMaxSTM);
            record.character.stamina[0] = static_cast<int16_t>(std::clamp(character.iSTM > 0 ? character.iSTM : stmMax, 1, 32767));
            record.character.stamina[1] = static_cast<int16_t>(std::clamp(stmMax, 1, 32767));
            std::fill(std::begin(record.character.resistance), std::end(record.character.resistance), int16_t{85});
            std::fill(std::begin(record.character.attackResistance), std::end(record.character.attackResistance), int16_t{85});
            record.character.lifeRegen = character.fHPRegen + character.fHPRegenBonus;
            record.character.manaRegen = character.fMPRegen + character.fMPRegenBonus;
            record.character.staminaRegen = character.fSPRegen + character.fSPRegenBonus;
            record.character.money = character.iGold;
            record.character.version[0] = 1;
            record.character.reformCode = characterReformCode(record.character);
            record.save.head = 150;
            record.save.playStage = character.iMapId;
            record.save.posX = static_cast<int32_t>(character.fPosX);
            record.save.posZ = static_cast<int32_t>(character.fPosZ);
            record.save.totalMoney = character.iGold;

            // The legacy transport reserves 32 bytes before the record data;
            // this matches rsRECORD_DBASE::SendRecordDataToClient().
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

    bool decodeClientAttackData2(std::vector<uint8_t>& payload, uint32_t objectSerial)
    {
        if (payload.size() != sizeof(int32_t) * 23 || objectSerial == 0)
            return false;

        int32_t frameSize = 0;
        int32_t code = 0;
        std::memcpy(&frameSize, payload.data(), sizeof(frameSize));
        std::memcpy(&code, payload.data() + sizeof(frameSize), sizeof(code));
        if (frameSize != static_cast<int32_t>(payload.size()) ||
            code != static_cast<int32_t>(Opcode::AttackDamage))
            return false;

        const uint32_t wordCount = static_cast<uint32_t>(payload.size() / sizeof(uint32_t));
        uint32_t encodedState = (static_cast<uint32_t>(code) ^ (static_cast<uint32_t>(code) << 7)) + wordCount * 6604u;
        uint32_t counter = (static_cast<uint32_t>(code) + objectSerial) & 0xFFCFu;
        const uint32_t increment = counter & 0x3Cu;

        // The original client encodes every DWORD after size/code in place.
        for (uint32_t index = 2; index < wordCount; ++index)
        {
            uint32_t encoded = 0;
            std::memcpy(&encoded, payload.data() + index * sizeof(uint32_t), sizeof(encoded));
            const uint32_t decoded = encoded ^ (counter << 13) ^ encodedState;
            std::memcpy(payload.data() + index * sizeof(uint32_t), &decoded, sizeof(decoded));
            encodedState = encoded;
            counter += counter + increment;
        }
        return true;
    }

    std::optional<Packet> toModernPacket(const std::vector<uint8_t> &payload)
    {
        if (payload.size() < sizeof(int32_t) * 2 || payload.size() > Constants::MAX_PKTSIZ)
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
                command.lParam < 0 || command.lParam > 64 || command.wParam < 0 || command.wParam > 64 ||
                (command.wParam != 0 && command.lParam >= command.wParam) ||
                (command.wParam == 0 && command.lParam != 0) || command.sParam <= 0 ||
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
            {
                LegacyNpcInfo player{};
                std::memcpy(&player, payload.data(), sizeof(player));

                // This is the player's own initial position. The serial in a
                // legacy PLAYERINFO frame may be zero or stale during server
                // selection, so never use it to select an entity. MovementHandler
                // binds LegacyPosition to the authenticated session instead.
                const int32_t area = -1;
                std::vector<uint8_t> movement(sizeof(int32_t) * 4);
                std::memcpy(movement.data(), &player.x, sizeof(player.x));
                std::memcpy(movement.data() + sizeof(int32_t), &player.y, sizeof(player.y));
                std::memcpy(movement.data() + sizeof(int32_t) * 2, &player.z, sizeof(player.z));
                std::memcpy(movement.data() + sizeof(int32_t) * 3, &area, sizeof(area));
                return Packet(static_cast<uint32_t>(Opcodes::LegacyPosition), std::move(movement));
            }
        case Opcode::GetPlayerInfo:
            if (command.size != sizeof(Command) || payload.size() != sizeof(Command) ||
                static_cast<uint32_t>(command.wParam) == 0)
                return std::nullopt;
            {
                PacketNpcClick click{};
                click.objectSerial = static_cast<uint32_t>(command.wParam);
                click.npcId = 0; // Resolve by serial; the legacy request has no NPC id.
                const auto *body = reinterpret_cast<const uint8_t *>(&click);
                return Packet(static_cast<uint32_t>(Opcodes::NpcUnitClick),
                              std::vector<uint8_t>(body, body + sizeof(click)));
            }
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
        case Opcode::Position:
        {
            // TRANS_PLAYPOS is the movement frame emitted by the original
            // client. It contains no trusted object serial; the session owns
            // the entity that receives the position.
            if (command.size != static_cast<int32_t>(sizeof(LegacyPlayPosition)) ||
                payload.size() != sizeof(LegacyPlayPosition))
                return std::nullopt;
            LegacyPlayPosition position{};
            std::memcpy(&position, payload.data(), sizeof(position));
            if (position.area < -1 || position.area > 255)
                return std::nullopt;

            std::vector<uint8_t> movement(sizeof(int32_t) * 4);
            std::memcpy(movement.data(), &position.x, sizeof(position.x));
            std::memcpy(movement.data() + sizeof(int32_t), &position.y, sizeof(position.y));
            std::memcpy(movement.data() + sizeof(int32_t) * 2, &position.z, sizeof(position.z));
            std::memcpy(movement.data() + sizeof(int32_t) * 3, &position.area, sizeof(position.area));
            return Packet(static_cast<uint32_t>(Opcodes::LegacyPosition), movement);
        }
        case Opcode::ProcessInfo:
            // TRANS_CHATMESSAGE carries two DWORDs followed by a bounded process path.
            if (command.size != static_cast<int32_t>(payload.size()) || command.size < 17 ||
                command.size > 512 || !isSafeString(reinterpret_cast<const char *>(payload.data() + 16), payload.size() - 16))
                return std::nullopt;
            modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
            break;
        case Opcode::ChatMessage:
            // The client sends normal chat as the compact form:
            // size, code, text (size = strlen(text) + 9). Server-originated
            // TRANS_CHATMESSAGE uses the 272-byte extended form.
            if (command.size != static_cast<int32_t>(payload.size()) || command.size < 9 || command.size > 272)
                return std::nullopt;
            {
                std::vector<uint8_t> body(sizeof(uint32_t) + 256);
                const size_t textOffset = payload.size() == 272 ? 16 : 8;
                const size_t textCapacity = payload.size() - textOffset;
                if (!isSafeString(reinterpret_cast<const char *>(payload.data() + textOffset), textCapacity))
                    return std::nullopt;
                if (payload.size() == 272)
                    std::memcpy(body.data(), payload.data() + 12, sizeof(uint32_t));
                const size_t textLength = std::min(textCapacity, body.size() - sizeof(uint32_t));
                std::memcpy(body.data() + sizeof(uint32_t), payload.data() + textOffset, textLength);
                return Packet(static_cast<uint32_t>(Opcodes::ChatMessage), body);
            }
        case Opcode::SaveThrowMoney:
            // The original client sends this to persist its current money on
            // the data server. Never trust it as an authoritative gold update;
            // the modern server persists gold through its own inventory path.
            if (command.size != static_cast<int32_t>(sizeof(Command)) ||
                payload.size() != sizeof(Command) || command.wParam < 0)
                return std::nullopt;
            modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
            break;
        case Opcode::PartyChatMessage:
            if (command.size != static_cast<int32_t>(payload.size()) ||
                command.size < 9 ||
                command.size > 512)
                return std::nullopt;
            // Some client builds send GM slash commands through the party
            // chat opcode. Accept only a slash command; ordinary party chat
            // remains rejected until its full domain layout is implemented.
            {
                size_t textOffset = payload.size();
                for (size_t i = 8; i < payload.size(); ++i) {
                    if (payload[i] == '/') { textOffset = i; break; }
                }
                if (textOffset == payload.size())
                    return Packet(static_cast<uint32_t>(Opcodes::LegacyNoop));
                std::vector<uint8_t> body(sizeof(uint32_t) + 256, 0);
                size_t textLength = 0;
                while (textOffset + textLength < payload.size() && textLength < 255) {
                    const auto ch = payload[textOffset + textLength];
                    if (ch == 0 || std::isspace(ch) || !std::isprint(ch)) break;
                    ++textLength;
                }
                if (textLength < 2) return Packet(static_cast<uint32_t>(Opcodes::LegacyNoop));
                std::memcpy(body.data() + sizeof(uint32_t), payload.data() + textOffset, textLength);
                return Packet(static_cast<uint32_t>(Opcodes::ChatMessage), body);
            }
        case Opcode::CommandUser:
            // smTRANS_COMMAND_EX: only the fixed 40-byte layout reaches the
            // GM service; authorization and command validation happen there.
            if (command.size != 40 || payload.size() != 40)
                return std::nullopt;
            return Packet(static_cast<uint32_t>(Opcodes::NetGameMasterCommand),
                          std::vector<uint8_t>(payload.begin() + sizeof(Command), payload.end()));
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
        case Opcode::WarningBlinkAtt:
            // The confirmed client layout is smTRANS_COMMAND (24 bytes). A
            // 92-byte frame with this opcode is not a known gameplay packet.
            if (command.size != static_cast<int32_t>(sizeof(Command)) ||
                payload.size() != sizeof(Command))
                return std::nullopt;
            modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
            break;
        case Opcode::EffectLowPet:
            // Legacy movement may carry a low-level-pet effect command. The
            // modern server has no pet state yet, but the bounded command must
            // not tear down an otherwise authenticated game session.
            if (command.size != static_cast<int32_t>(payload.size()) || command.size < static_cast<int32_t>(sizeof(Command)) ||
                command.size > 512)
                return std::nullopt;
            modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
            break;
        case Opcode::InventoryPosition:
        case Opcode::InventoryPosition2:
            // Never trust the client copy as an inventory update. Accept only
            // the exact fixed layout so valid clients are not disconnected.
            if (command.size != static_cast<int32_t>(sizeof(LegacyInventoryPosition)) ||
                payload.size() != sizeof(LegacyInventoryPosition))
                return std::nullopt;
            {
                LegacyInventoryPosition inventory{};
                std::memcpy(&inventory, payload.data(), sizeof(inventory));
                if (inventory.itemCount < 0 || inventory.itemCount > 16)
                    return std::nullopt;
            }
            modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
            break;
        default:
            if (command.code == 194) // OPCODE_SHOPTITEM / TRANS_BUY_SHOPITEM
            {
                if (payload.size() != sizeof(LegacyBuyShopItem) || command.size != static_cast<int32_t>(payload.size()))
                    return std::nullopt;
                LegacyBuyShopItem request{};
                std::memcpy(&request, payload.data(), sizeof(request));
                // sITEMINFO::CODE is at offset 24 inside the x86 sITEMINFO.
                uint32_t itemCode = 0;
                std::memcpy(&itemCode, request.item.itemInfo + 24, sizeof(itemCode));
                if (itemCode == 0 || request.itemCount <= 0 || request.itemCount > 2000000000)
                    return std::nullopt;
                PacketNpcShopRequest modern{static_cast<int32_t>(itemCode), request.itemCount, 0};
                const auto *bytes = reinterpret_cast<const uint8_t *>(&modern);
                return Packet(static_cast<uint32_t>(Opcodes::BuyItemNPCShop),
                              std::vector<uint8_t>(bytes, bytes + sizeof(modern)));
            }
            if (command.code == 195) // OPCODE_SHOP_SELLITEM / TRANS_ITEMINFO
            {
                if (payload.size() != sizeof(LegacySellShopItem) || command.size != static_cast<int32_t>(payload.size()))
                    return std::nullopt;
                LegacySellShopItem request{};
                std::memcpy(&request, payload.data(), sizeof(request));
                uint32_t itemCode = 0;
                std::memcpy(&itemCode, request.itemInfo + 24, sizeof(itemCode));
                if (itemCode == 0)
                    return std::nullopt;
                PacketNpcShopRequest modern{static_cast<int32_t>(itemCode), 1, 0};
                const auto *bytes = reinterpret_cast<const uint8_t *>(&modern);
                return Packet(static_cast<uint32_t>(Opcodes::SellItemNPCShop),
                              std::vector<uint8_t>(bytes, bytes + sizeof(modern)));
            }
            // Skill attack frames are accepted only as structurally valid
            // control traffic until the server-side skill catalog is present.
            // They must not reach gameplay with client-supplied damage.
            if (command.code == 41) // OPCODE_SKIL_ATTACKDATA
            {
                if (payload.size() < offsetof(LegacySkillAttackData, targetSerials) ||
                    command.size != static_cast<int32_t>(payload.size()))
                    return std::nullopt;
                LegacySkillAttackData skill{};
                std::memcpy(&skill, payload.data(), std::min(payload.size(), sizeof(skill)));
                const size_t expected = offsetof(LegacySkillAttackData, targetSerials) +
                                        static_cast<size_t>(std::max(0, skill.targetCount)) * sizeof(uint32_t);
                if (skill.destObjectSerial == 0 || skill.targetCount < 0 || skill.targetCount > 42 ||
                    skill.attackSize < 0 || expected != payload.size())
                    return std::nullopt;
                modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
                break;
            }
            if (command.code == 216) // OPCODE_ATTACKDATA2 carrying skill data
            {
                // 92-byte frames are the ordinary TRANS_ATTACKDATA2 and are
                // handled below; only the larger skill layout enters here.
                if (payload.size() >= offsetof(LegacySkillAttackData2, targetSerials))
                {
                    if (command.size != static_cast<int32_t>(payload.size()))
                        return std::nullopt;
                    LegacySkillAttackData2 skill{};
                    std::memcpy(&skill, payload.data(), std::min(payload.size(), sizeof(skill)));
                    const size_t expected = offsetof(LegacySkillAttackData2, targetSerials) +
                                            static_cast<size_t>(std::max(0, skill.targetCount)) * sizeof(uint32_t);
                    if (skill.destObjectSerial == 0 || skill.targetCount < 0 || skill.targetCount > 42 ||
                        skill.attackSize < 0 || expected != payload.size())
                        return std::nullopt;
                    modernOpcode = static_cast<uint32_t>(Opcodes::LegacyNoop);
                    break;
                }
            }
            if (command.code == static_cast<int32_t>(Opcode::AttackData) &&
                command.size == static_cast<int32_t>(sizeof(LegacyAttackData)) &&
                payload.size() >= sizeof(LegacyAttackData))
            {
                LegacyAttackData attack{};
                std::memcpy(&attack, payload.data(), sizeof(attack));
                if (attack.destObjectSerial == 0 || attack.targetObjectSerial == 0 || attack.attackSize < 0)
                    return std::nullopt;
                // Keep the complete legacy layout so nearby legacy clients
                // receive the original coordinates, state and checksums.
                std::vector<uint8_t> body(sizeof(attack) - sizeof(int32_t) * 2);
                std::memcpy(body.data(), payload.data() + sizeof(int32_t) * 2, body.size());
                return Packet(static_cast<uint32_t>(Opcodes::AttackData), body);
            }
            if (command.code == static_cast<int32_t>(Opcode::AttackDamage) &&
                command.size == static_cast<int32_t>(sizeof(LegacyAttackData2)) &&
                payload.size() == sizeof(LegacyAttackData2))
            {
                LegacyAttackData2 attack{};
                std::memcpy(&attack, payload.data(), sizeof(attack));
                if (attack.code != static_cast<int32_t>(Opcode::AttackDamage) ||
                    attack.destObjectSerial == 0 || attack.targetObjectSerial == 0 ||
                    attack.attackSize < 0 || attack.attackCount < 0 || attack.attackCount > 256 ||
                    attack.checkSum != legacyAttackChecksum(attack))
                    return std::nullopt;
                std::vector<uint8_t> body(sizeof(attack) - sizeof(int32_t) * 2);
                std::memcpy(body.data(), payload.data() + sizeof(int32_t) * 2, body.size());
                return Packet(static_cast<uint32_t>(Opcodes::AttackData2), std::move(body));
            }
            if (command.code == static_cast<int32_t>(Opcode::AttackData2) &&
                command.size == static_cast<int32_t>(sizeof(LegacyAttackData2)) &&
                payload.size() >= sizeof(LegacyAttackData2))
            {
                LegacyAttackData2 attack{};
                std::memcpy(&attack, payload.data(), sizeof(attack));
                if (attack.destObjectSerial == 0 || attack.targetObjectSerial == 0 || attack.attackSize < 0)
                    return std::nullopt;
                std::vector<uint8_t> body(sizeof(attack) - sizeof(int32_t) * 2);
                std::memcpy(body.data(), payload.data() + sizeof(int32_t) * 2, body.size());
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
                    modern.LParam = internalClassForLegacyJobCode(modern.LParam);
                    if (modern.LParam == 0)
                        return std::nullopt;
                }
                if (command.code != 5)
                    std::memcpy(modern.szAccount, payload.data() + 20, 21);
                const size_t nameOffset = command.code == 5 ? 20 : 41;
                bool nameCopied = copyLegacyCharacterName(modern.szName, sizeof(modern.szName), payload, nameOffset);
                // Some compatible clients serialize the command without the
                // account field. Accept only that exact alternate offset.
                if (!nameCopied && command.code != 5)
                    nameCopied = copyLegacyCharacterName(modern.szName, sizeof(modern.szName), payload, 20);
                if (!nameCopied)
                    return std::nullopt;
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
            if (result == 1)                   // LoginResult::Success
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
            // PacketServerList possui somente quatro entradas; nunca use o
            // limite maior do frame legado para indexar o pacote moderno.
            legacy.count = std::clamp(modern.iGameServers, 0, 4);
            for (int i = 0; i < legacy.count; ++i)
            {
                std::memcpy(legacy.servers[i].name, modern.sServers[i].szName, sizeof(legacy.servers[i].name));
                std::memcpy(legacy.servers[i].ip1, modern.sServers[i].szaIP[0], sizeof(legacy.servers[i].ip1));
                std::memcpy(legacy.servers[i].ip2, modern.sServers[i].szaIP[1], sizeof(legacy.servers[i].ip2));
                std::memcpy(legacy.servers[i].ip3, modern.sServers[i].szaIP[2], sizeof(legacy.servers[i].ip3));
                // The legacy client opens a new socket to the selected game
                // server. Preserve the configured endpoint instead of sending
                // it back to the login port.
                const auto validPort = [](int32_t port) -> uint32_t
                {
                    return port > 0 && port <= 65535 ? static_cast<uint32_t>(port) : 5056u;
                };
                legacy.servers[i].port1 = validPort(modern.sServers[i].iaPort[0]);
                legacy.servers[i].port2 = validPort(modern.sServers[i].iaPort[1]);
                legacy.servers[i].port3 = validPort(modern.sServers[i].iaPort[2]);
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
                // AccountService/CharacterHandler already expose the legacy
                // display code in JobCode. Mapping it again swaps classes 5-8.
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
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::NpcData))
        {
            if (packet.body.size() != sizeof(PacketNpcData))
                return std::nullopt;
            PacketNpcData npc{};
            std::memcpy(&npc, packet.body.data(), sizeof(npc));
            if (npc.npcId <= 0 || npc.objectSerial == 0 || npc.enabled == 0 ||
                !isSafeString(npc.szName, sizeof(npc.szName)) ||
                !isSafeString(npc.szModelName, sizeof(npc.szModelName)))
                return std::nullopt;

            // O cliente legado só aloca o NPC ao receber PLAYDATAGROUP. O
            // PLAYERINFO sozinho é descartado porque ainda não existe um
            // slot em chrOtherPlayer para recebê-lo.
            if (requestCode != static_cast<int32_t>(Opcode::GetPlayerInfo))
            {
                LegacyPlayDataGroup group{
                    static_cast<int32_t>(sizeof(LegacyPlayDataGroup) + sizeof(LegacyPlayData)),
                    static_cast<int32_t>(Opcode::PlayDataGroup), 1, 0x80000000u};
                LegacyPlayData play{};
                play.size = sizeof(play);
                play.code = static_cast<int32_t>(Opcode::PlayData1);
                play.objectSerial = npc.objectSerial;
                play.x = npc.x;
                play.y = npc.y;
                play.z = npc.z;
                play.angle[1] = static_cast<int16_t>(npc.angleY);
                play.autoCharacterCode = npc.objectSerial;
                append(&group, sizeof(group));
                append(&play, sizeof(play));
                return payload;
            }

            LegacyNpcInfo legacy{};
            legacy.size = sizeof(legacy);
            legacy.code = static_cast<int32_t>(Opcode::PlayerInfo);
            legacy.character.objectSerial = npc.objectSerial;
            legacy.character.state = 0; // smCHAR_STATE_NPC
            legacy.character.brood = 0xA0;
            // SetNextNpcPattern() in the original client forces NPCs to the
            // normal pattern size before enabling their display.
            legacy.character.sizeLevel = 1;
            legacy.character.level = 1;
            legacy.character.life[0] = legacy.character.life[1] = 100;
            legacy.character.mana[0] = legacy.character.mana[1] = 100;
            legacy.character.stamina[0] = legacy.character.stamina[1] = 100;
            legacy.character.weight[1] = 100;
            legacy.character.moveSpeed = 250;
            std::memcpy(legacy.character.name, npc.szName, sizeof(legacy.character.name));
            std::memcpy(legacy.character.modelName, npc.szModelName, sizeof(legacy.character.modelName));
            // Fortress fills only the NPC body model. An empty second model
            // prevents the legacy loader from trying to load the body twice.
            legacy.character.resistance[0] = 85;
            legacy.character.resistance[1] = 85;
            legacy.objectSerial = npc.objectSerial;
            legacy.x = npc.x;
            legacy.y = npc.y;
            legacy.z = npc.z;
            legacy.ay = npc.angleY;
            append(&legacy, sizeof(legacy));
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::Shop_ItemList))
        {
            if (packet.body.size() < sizeof(PacketNpcShopListHeader))
                return std::nullopt;

            PacketNpcShopListHeader header{};
            std::memcpy(&header, packet.body.data(), sizeof(header));
            if (header.npcId <= 0 || header.itemKind < 1 || header.itemKind > 3 ||
                header.itemCount <= 0 || header.itemCount > 200 ||
                packet.body.size() != sizeof(header) + static_cast<size_t>(header.itemCount) * sizeof(PacketNpcShopListEntry))
                return std::nullopt;

            struct LegacyShopHeader
            {
                int32_t size, code, itemKind, itemCount;
                int32_t reserved[4];
            } legacy{};
            legacy.code = static_cast<int32_t>(Opcode::ShopItemList);
            legacy.itemKind = header.itemKind;
            legacy.itemCount = header.itemCount;

            std::vector<uint8_t> itemData;
            itemData.reserve(static_cast<size_t>(header.itemCount) * LegacyItemInfoSize);
            for (int32_t i = 0; i < header.itemCount; ++i) {
                PacketNpcShopListEntry entry{};
                const size_t offset = sizeof(header) + static_cast<size_t>(i) * sizeof(entry);
                std::memcpy(&entry, packet.body.data() + offset, sizeof(entry));
                if (entry.itemCode <= 0 || entry.buyPrice < 0 || entry.sellPrice < 0 ||
                    entry.requiredLevel < 1 || !isSafeString(entry.itemName, sizeof(entry.itemName)))
                    return std::nullopt;
                auto encoded = encodeLegacyShopItem(entry, header.itemKind);
                itemData.insert(itemData.end(), encoded.begin(), encoded.end());
            }
            const size_t totalSize = sizeof(legacy) + itemData.size();
            if (totalSize > LegacyPacketCodec::MaxFrameSize)
                return std::nullopt;
            legacy.size = static_cast<int32_t>(totalSize);
            append(&legacy, sizeof(legacy));
            payload.insert(payload.end(), itemData.begin(), itemData.end());
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::OpenNPC))
        {
            if (packet.body.size() != sizeof(PacketNpcOpen))
                return std::nullopt;
            PacketNpcOpen opened{};
            std::memcpy(&opened, packet.body.data(), sizeof(opened));
            if (opened.npcId <= 0 || opened.objectSerial == 0 || opened.mapId < 0)
                return std::nullopt;
            // There is no confirmed generic legacy UI frame for this response.
            // The validated click is accepted, but no guessed frame is emitted.
            return std::vector<uint8_t>{};
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::RecordResult) ||
                 packet.header.opcode == static_cast<uint32_t>(Opcodes::FailRecordData))
        {
            if (packet.body.size() < sizeof(PacketRecordResult) - sizeof(PacketHeader))
                return std::nullopt;
            PacketRecordResult result{};
            std::memcpy(reinterpret_cast<uint8_t *>(&result) + sizeof(PacketHeader), packet.body.data(),
                        sizeof(result) - sizeof(PacketHeader));
            // A resposta de selecao ja carrega o personagem completo. Nao dependa
            // do requestCode aqui: o cliente envia keep-alive logo depois da
            // selecao e esse valor pode ter sido atualizado antes da resposta.
            if (packet.header.opcode == static_cast<uint32_t>(Opcodes::RecordResult) &&
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
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::AttackData) ||
                 packet.header.opcode == static_cast<uint32_t>(Opcodes::AttackData2))
        {
            const bool secondLayout = packet.header.opcode == static_cast<uint32_t>(Opcodes::AttackData2);
            const size_t fullSize = secondLayout ? sizeof(LegacyAttackData2) : sizeof(LegacyAttackData);
            if (packet.body.size() != fullSize - sizeof(int32_t) * 2)
                return std::nullopt;
            std::vector<uint8_t> frame(fullSize, 0);
            const int32_t legacyCode = secondLayout ? static_cast<int32_t>(Opcode::AttackData2)
                                                    : static_cast<int32_t>(Opcode::AttackData);
            const int32_t frameSize = static_cast<int32_t>(fullSize);
            std::memcpy(frame.data(), &frameSize, sizeof(frameSize));
            std::memcpy(frame.data() + sizeof(frameSize), &legacyCode, sizeof(legacyCode));
            std::memcpy(frame.data() + sizeof(int32_t) * 2, packet.body.data(), packet.body.size());
            if (secondLayout)
            {
                LegacyAttackData2 attack{};
                std::memcpy(&attack, frame.data(), sizeof(attack));
                if (attack.destObjectSerial == 0 || attack.targetObjectSerial == 0 || attack.attackSize < 0)
                    return std::nullopt;
            }
            else
            {
                LegacyAttackData attack{};
                std::memcpy(&attack, frame.data(), sizeof(attack));
                if (attack.destObjectSerial == 0 || attack.targetObjectSerial == 0 || attack.attackSize < 0)
                    return std::nullopt;
            }
            return frame;
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
            struct LegacyChatBuffer
            {
                int32_t size, code;
                uint32_t ip;
            } legacy{};
            const auto messageLength = strnlen_s(reinterpret_cast<const char *>(packet.body.data() + sizeof(uint32_t)), 256);
            legacy.size = static_cast<int32_t>(sizeof(legacy) + messageLength + 1);
            legacy.code = static_cast<int32_t>(Opcode::ChatMessage);
            const auto* message = packet.body.data() + sizeof(uint32_t);
            std::vector<uint8_t> legacyBytes(static_cast<size_t>(legacy.size), 0);
            std::memcpy(legacyBytes.data(), &legacy, sizeof(legacy));
            std::memcpy(legacyBytes.data() + sizeof(legacy), message, messageLength + 1);
            append(legacyBytes.data(), legacyBytes.size());
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::UnitStatusMove) ||
                 packet.header.opcode == static_cast<uint32_t>(Opcodes::PlayData))
        {
            if (packet.body.size() != sizeof(int32_t) * 4)
                return std::nullopt;
            LegacyPlayData legacy{};
            legacy.size = sizeof(legacy);
            legacy.code = static_cast<int32_t>(Opcode::PlayData1);
            std::memcpy(&legacy.objectSerial, packet.body.data(), sizeof(legacy.objectSerial));
            std::memcpy(&legacy.x, packet.body.data() + 4, sizeof(legacy.x));
            std::memcpy(&legacy.y, packet.body.data() + 8, sizeof(legacy.y));
            std::memcpy(&legacy.z, packet.body.data() + 12, sizeof(legacy.z));
            legacy.autoCharacterCode = legacy.objectSerial;
            append(&legacy, sizeof(legacy));
        }
        else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::RegenChar))
        {
            if (packet.body.size() != sizeof(int32_t) * 4)
                return std::nullopt;
            struct LegacyRegen
            {
                int32_t length, header, unk, amount, unk3, unk4;
            } legacy{};
            legacy.length = sizeof(legacy);
            legacy.header = static_cast<int32_t>(Opcodes::RegenChar);
            std::memcpy(&legacy.unk, packet.body.data(), sizeof(int32_t) * 4);
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
