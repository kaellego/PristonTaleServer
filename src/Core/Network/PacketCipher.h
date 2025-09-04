#pragma once

#include <cstdint>
#include <vector>

// Forward declaration para a struct Packet
struct Packet;

/**
 * @class PacketCipher
 * @brief Gerencia a criptografia e descriptografia de pacotes para uma única sessão.
 *
 * Esta classe encapsula o estado criptográfico (conjunto de chaves, próximo índice)
 * e os algoritmos para codificar e decodificar os dados dos pacotes.
 */
class PacketCipher {
public:
    PacketCipher();

    /**
     * @brief Gera o pacote inicial de troca de chaves a ser enviado ao cliente.
     * @param port A porta do servidor, usada para ofuscar a chave.
     * @return um vetor de bytes representando o pacote KeySet completo.
     */
    std::vector<uint8_t> generateKeySetPacket(uint16_t port);

    /**
     * @brief Processa um pacote de troca de chaves recebido de um cliente (se necessário).
     * @param packet O pacote KeySet recebido.
     * @param port A porta do servidor, usada para desofuscar a chave.
     */
    void receiveKeySet(const Packet& packet, uint16_t port);

    /**
     * @brief Criptografa um pacote de saída in-place.
     * @param packet O pacote a ser criptografado.
     */
    void encrypt(Packet& packet);

    /**
     * @brief Descriptografa um pacote de entrada in-place.
     * @param packet O pacote a ser descriptografado.
     */
    void decrypt(Packet& packet);

    bool isKeySet() const { return m_isKeySet; }

private:
    static constexpr int NUM_ENCKEYS = 256;

    bool m_isKeySet = false;
    uint8_t m_keySet[NUM_ENCKEYS];
    uint8_t m_nextKeyIndex = 0;
};