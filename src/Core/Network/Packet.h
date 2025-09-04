#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

#pragma pack(push, 1)

/**
 * @struct PacketHeader
 * @brief O cabe�alho padr�o, AGORA COM CAMPOS PARA CRIPTOGRAFIA.
 */
struct PacketHeader {
    uint16_t length;       // 2 bytes - Comprimento total do pacote
    uint8_t  iEncKeyIndex; // 1 byte  - �ndice da chave de criptografia
    uint8_t  iEncrypted;   // 1 byte  - Flag que indica se est� criptografado
    uint32_t opcode;       // 4 bytes - O Opcode real (era o iHeader)
};

#pragma pack(pop)

struct Packet {
    PacketHeader header;
    std::vector<uint8_t> body;

    Packet() = default;

    /**
     * @brief Constr�i um pacote a partir de um opcode e um corpo.
     */
    Packet(uint32_t op, const std::vector<uint8_t>& packetBody = {})
        : body(packetBody) {

        header.opcode = op;
        header.iEncKeyIndex = 0;
        header.iEncrypted = 0;

        size_t total_size = sizeof(PacketHeader) + body.size();
        if (total_size > UINT16_MAX) {
            throw std::runtime_error("Tamanho do pacote excede o limite maximo de 64KB.");
        }
        header.length = static_cast<uint16_t>(total_size);
    }

    /**
     * @brief NOVO CONSTRUTOR: Constr�i um pacote a partir de um cabe�alho e corpo j� existentes.
     */
    Packet(const PacketHeader& packetHeader, const std::vector<uint8_t>& packetBody)
        : header(packetHeader), body(packetBody) {}
};