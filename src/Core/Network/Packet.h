#pragma once

#include <cstdint>
#include <vector>
#include <stdexcept>

// Define o alinhamento de 1 byte para as structs de pacote.
// Isso garante que o compilador não adicione preenchimento (padding)
// entre os membros, fazendo com que o layout da struct na memória
// corresponda exatamente ao formato enviado pela rede.
#pragma pack(push, 1)

/**
 * @struct PacketHeader
 * @brief O cabeçalho padrão para todos os pacotes de rede do jogo.
 */
struct PacketHeader {
    uint16_t length; // Comprimento total do pacote (cabeçalho + corpo)
    uint16_t opcode; // O identificador do tipo de pacote
};

#pragma pack(pop)


/**
 * @struct Packet
 * @brief Representa um pacote de rede completo, com cabeçalho e dados.
 */
struct Packet {
    PacketHeader header;
    std::vector<uint8_t> body; // O corpo do pacote com os dados variáveis

    /**
     * @brief Constrói um pacote vazio.
     */
    Packet() = default;

    /**
     * @brief Constrói um pacote a partir de um opcode e um corpo de dados.
     * @throws std::runtime_error se o tamanho total do pacote exceder 65535 bytes.
     */
    Packet(uint16_t opcode, const std::vector<uint8_t>& packetBody = {})
        : body(packetBody) {

        header.opcode = opcode;
        size_t total_size = sizeof(PacketHeader) + body.size();

        // Verifica se o tamanho cabe em um uint16_t
        if (total_size > UINT16_MAX) { // UINT16_MAX está definido em <cstdint>
            throw std::runtime_error("Tamanho do pacote excede o limite maximo de 64KB.");
        }

        // A conversão agora é segura e o aviso desaparecerá
        header.length = static_cast<uint16_t>(total_size);
    }
};