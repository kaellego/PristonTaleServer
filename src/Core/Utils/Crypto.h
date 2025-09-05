#pragma once

#include <string>
#include <algorithm>
#include <cctype> // Para std::tolower

// picosha2.h não é mais necessário aqui, mas pode ser útil para outras coisas no futuro.
// #include "picosha2.h" 

namespace Crypto {

    /**
     * @brief Valida um hash de senha recebido contra um hash armazenado.
     * Realiza uma comparação de string case-insensitive.
     * @param receivedHash O hash enviado pelo cliente (já no formato "usuario:senha" com hash).
     * @param databaseHash O hash SHA-256 hexadecimal armazenado no banco de dados.
     * @return true se os hashes corresponderem, false caso contrário.
     */
    inline bool validatePassword(const std::string& receivedHash, const std::string& databaseHash) {
        // _stricmp é uma função do Windows para comparar strings sem diferenciar maiúsculas/minúsculas.
        // Se estiver em outra plataforma, use strcasecmp.
        return _stricmp(receivedHash.c_str(), databaseHash.c_str()) == 0;
    }

} // namespace Crypto