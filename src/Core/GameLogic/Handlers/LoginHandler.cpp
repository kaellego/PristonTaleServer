#include "GameLogic/Handlers/LoginHandler.h"
#include "GameLogic/Services/AccountService.h"
#include "Network/ClientSession.h"
#include "Shared/datatypes.h"
#include "Logging/LogService.h"
#include <cstring> // Para std::memcpy

LoginHandler::LoginHandler(AccountService& accountService, LogService& logService)
    : m_accountService(accountService), m_logService(logService) {}

void LoginHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    // Calcula o tamanho esperado do corpo do pacote de login
    constexpr size_t expectedBodySize = sizeof(PacketLoginUser) - sizeof(PacketHeader);

    if (packet.body.size() < expectedBodySize) {
        m_logService.warn("Pacote de login recebido com tamanho invalido. Esperado >= {}, recebido {}.",
            expectedBodySize, packet.body.size());
        session->close();
        return;
    }

    // --- L�GICA DE RECONSTRU��O CORRIGIDA ---
    PacketLoginUser loginData;
    // 1. Copia o cabe�alho
    loginData.header = packet.header;

    // 2. Copia o corpo para o resto da struct
    //    Isso � seguro porque controlamos o layout da mem�ria com #pragma pack
    std::memcpy(
        reinterpret_cast<uint8_t*>(&loginData) + sizeof(PacketHeader), // Destino: o endere�o ap�s o cabe�alho
        packet.body.data(),                                           // Origem: o buffer do corpo
        expectedBodySize                                              // Tamanho: o tamanho do corpo
    );

    // Envia a requisi��o para a fila de processamento do AccountService
    m_accountService.queueLoginRequest(session, loginData);
}