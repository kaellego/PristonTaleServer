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
    uint16_t length;
    uint16_t opcode;
    uint8_t iEncKeyIndex; // Índice da chave de criptografia usada
    uint8_t iEncrypted;   // Flag que indica se o pacote está criptografado (0 ou 1)
};

#pragma pack(pop)

struct Packet {
    PacketHeader header;
    std::vector<uint8_t> body;

    Packet() = default;

    /**
     * @brief Constrói um pacote a partir de um opcode e um corpo.
     */
    Packet(uint16_t opcode, const std::vector<uint8_t>& packetBody = {})
        : body(packetBody) {

        header.opcode = opcode;
        header.iEncKeyIndex = 0; // Padrão
        header.iEncrypted = 0;   // Padrão

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