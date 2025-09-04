#pragma once

#include <string>
#include <algorithm> // Para std::transform
#include "picosha2.h"

namespace Crypto {

    /**
     * @brief Valida uma senha em texto puro contra um hash SHA-256 armazenado.
     * Constrói a string "usuario:senha", a converte para minúsculas,
     * gera o hash e o compara com o hash do banco de dados.
     * @param accountName O nome da conta.
     * @param plainPassword A senha em texto puro enviada pelo cliente.
     * @param hashedPassword O hash SHA-256 hexadecimal armazenado no banco de dados.
     * @return true se a senha for válida, false caso contrário.
     */
    inline bool validatePassword(const std::string& accountName, const std::string& plainPassword, const std::string& hashedPassword) {
        // 1. Constrói a string original no formato "usuario:senha"
        std::string original_string = accountName + ":" + plainPassword;

        // 2. Converte a string para minúsculas (a maioria dos hashes de PT são case-insensitive)
        std::transform(original_string.begin(), original_string.end(), original_string.begin(),
            [](unsigned char c) { return std::tolower(c); });

        // 3. Gera o hash SHA-256 da string formatada
        std::string generated_hash = picosha2::hash256_hex_string(original_string);

        // 4. Compara o hash gerado com o hash do banco de dados (case-insensitive)
        return _stricmp(generated_hash.c_str(), hashedPassword.c_str()) == 0;
    }

} // namespace Crypto