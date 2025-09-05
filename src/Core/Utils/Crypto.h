#pragma once

#include <string>
#include <algorithm>
#include <cctype> // Para std::tolower

// picosha2.h n�o � mais necess�rio aqui, mas pode ser �til para outras coisas no futuro.
// #include "picosha2.h" 

namespace Crypto {

    /**
     * @brief Valida um hash de senha recebido contra um hash armazenado.
     * Realiza uma compara��o de string case-insensitive.
     * @param receivedHash O hash enviado pelo cliente (j� no formato "usuario:senha" com hash).
     * @param databaseHash O hash SHA-256 hexadecimal armazenado no banco de dados.
     * @return true se os hashes corresponderem, false caso contr�rio.
     */
    inline bool validatePassword(const std::string& receivedHash, const std::string& databaseHash) {
        // _stricmp � uma fun��o do Windows para comparar strings sem diferenciar mai�sculas/min�sculas.
        // Se estiver em outra plataforma, use strcasecmp.
        return _stricmp(receivedHash.c_str(), databaseHash.c_str()) == 0;
    }

} // namespace Crypto