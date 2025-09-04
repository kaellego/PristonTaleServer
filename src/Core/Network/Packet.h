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
    uint16_t length;
    uint16_t opcode;
    uint8_t iEncKeyIndex; // �ndice da chave de criptografia usada
    uint8_t iEncrypted;   // Flag que indica se o pacote est� criptografado (0 ou 1)
};

#pragma pack(pop)

struct Packet {
    PacketHeader header;
    std::vector<uint8_t> body;

    Packet() = default;

    /**
     * @brief Constr�i um pacote a partir de um opcode e um corpo.
     */
    Packet(uint16_t opcode, const std::vector<uint8_t>& packetBody = {})
        : body(packetBody) {

        header.opcode = opcode;
        header.iEncKeyIndex = 0; // Padr�o
        header.iEncrypted = 0;   // Padr�o

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