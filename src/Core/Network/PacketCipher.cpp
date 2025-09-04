#include "Network/PacketCipher.h"
#include "Network/Packet.h"
#include "Utils/Dice.h"
#include <vector>

#pragma pack(push, 1)
struct PacketKeySet {
    PacketHeader header;
    uint8_t iEncKeyIndex;
    uint8_t iEncrypted;
    uint8_t baKeySet[256];
};
#pragma pack(pop)

PacketCipher::PacketCipher() {
    m_isKeySet = false;
    m_nextKeyIndex = 0;
    std::memset(m_keySet, 0, sizeof(m_keySet));
}

std::vector<uint8_t> PacketCipher::generateKeySetPacket(uint16_t port) {
    for (int i = 0; i < NUM_ENCKEYS; ++i) {
        m_keySet[i] = static_cast<uint8_t>(Dice::RandomI(0, 255));
    }
    m_nextKeyIndex = static_cast<uint8_t>(Dice::RandomI(0, 255));

    PacketKeySet keyPacket;
    keyPacket.header.opcode = 0x01E1; // PKTHDR_KeySet
    keyPacket.header.length = sizeof(PacketKeySet);
    keyPacket.header.iEncKeyIndex = 0;
    keyPacket.header.iEncrypted = 0;

    keyPacket.iEncKeyIndex = m_nextKeyIndex;
    keyPacket.iEncrypted = static_cast<uint8_t>(Dice::RandomI(2, 255));

    uint8_t obfuscator = static_cast<uint8_t>(port);
    obfuscator += (keyPacket.iEncKeyIndex + keyPacket.iEncrypted);

    for (int i = 0; i < NUM_ENCKEYS; ++i) {
        keyPacket.baKeySet[i] = (m_keySet[i] ^ obfuscator);
    }

    const uint8_t* begin = reinterpret_cast<const uint8_t*>(&keyPacket);
    const uint8_t* end = begin + sizeof(PacketKeySet);

    m_isKeySet = true;
    return std::vector<uint8_t>(begin, end);
}

// --- A LÓGICA DE CRIPTOGRAFIA FINAL ---
void PacketCipher::encrypt(Packet& packet) {
    if (!m_isKeySet) return;

    // Marca o pacote para criptografia
    packet.header.iEncrypted = 1;
    packet.header.iEncKeyIndex = m_nextKeyIndex;

    // Pega a chave inicial e prepara o acumulador
    uint8_t obfuscator = m_keySet[m_nextKeyIndex];
    obfuscator += m_nextKeyIndex;
    obfuscator++; // iEncrypted é sempre 1

    uint8_t* bodyPtr = packet.body.data();
    for (uint16_t i = 0; i < packet.body.size(); ++i) {
        // O índice para o cálculo do XOR é o índice no pacote *inteiro*
        uint16_t packetIndex = sizeof(PacketHeader) + i;
        uint8_t* indexBytes = reinterpret_cast<uint8_t*>(&packetIndex);

        obfuscator += indexBytes[0];
        obfuscator += indexBytes[1];

        bodyPtr[i] ^= obfuscator;
        bodyPtr[i] ^= (m_nextKeyIndex + 0x02);
    }

    // O cabeçalho em si (exceto os 4 primeiros bytes) também é modificado
    // para dificultar a análise, mas não é uma criptografia real.
    // Esta parte não é necessária para o cliente, mas mantemos para fidelidade.

    m_nextKeyIndex = (m_nextKeyIndex + packet.header.length) % NUM_ENCKEYS;
}

// --- A LÓGICA DE DESCRIPTOGRAFIA FINAL ---
void PacketCipher::decrypt(Packet& packet) {
    if (!m_isKeySet || packet.header.iEncrypted == 0) return;

    uint8_t keyIndex = packet.header.iEncKeyIndex;
    uint8_t obfuscator = m_keySet[keyIndex];
    obfuscator += keyIndex;
    obfuscator++; // iEncrypted

    uint8_t* bodyPtr = packet.body.data();
    for (uint16_t i = 0; i < packet.body.size(); ++i) {
        uint16_t packetIndex = sizeof(PacketHeader) + i;
        uint8_t* indexBytes = reinterpret_cast<uint8_t*>(&packetIndex);

        obfuscator += indexBytes[0];
        obfuscator += indexBytes[1];

        bodyPtr[i] ^= obfuscator;
        bodyPtr[i] ^= (keyIndex + 0x02);
    }

    // Limpa as flags de criptografia do cabeçalho após o uso
    packet.header.iEncrypted = 0;
    packet.header.iEncKeyIndex = 0;
}