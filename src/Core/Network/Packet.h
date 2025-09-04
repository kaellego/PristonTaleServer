#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

#pragma pack(push, 1)

/**
 * @struct PacketHeader
 * @brief O cabeçalho padrão, AGORA COM CAMPOS PARA CRIPTOGRAFIA.
 */
struct PacketHeader {
    uint16_t length;       // 2 bytes - Comprimento total do pacote
    uint8_t  iEncKeyIndex; // 1 byte  - Índice da chave de criptografia
    uint8_t  iEncrypted;   // 1 byte  - Flag que indica se está criptografado
    uint32_t opcode;       // 4 bytes - O Opcode real (era o iHeader)
};

#pragma pack(pop)

struct Packet {
    PacketHeader header;
    std::vector<uint8_t> body;

    Packet() = default;

    /**
     * @brief Constrói um pacote a partir de um opcode e um corpo.
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
     * @brief NOVO CONSTRUTOR: Constrói um pacote a partir de um cabeçalho e corpo já existentes.
     */
    Packet(const PacketHeader& packetHeader, const std::vector<uint8_t>& packetBody)
        : header(packetHeader), body(packetBody) {}
};