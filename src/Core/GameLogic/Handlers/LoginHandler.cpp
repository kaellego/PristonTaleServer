#include "GameLogic/Handlers/LoginHandler.h"
#include "GameLogic/Services/AccountService.h"
#include "Network/ClientSession.h"
#include "Shared/datatypes.h"
#include "Logging/LogService.h"
#include <cstring> // Para std::memcpy

LoginHandler::LoginHandler(AccountService& accountService, LogService& logService)
    : m_accountService(accountService), m_logService(logService) {}

void LoginHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    // O tamanho do corpo do pacote agora deve ser o tamanho total da struct de login MENOS o cabeçalho
    constexpr size_t expectedBodySize = sizeof(PacketLoginUser) - sizeof(PacketHeader);

    if (packet.body.size() != expectedBodySize) {
        m_logService.warn("Pacote de login recebido com tamanho de corpo invalido. Esperado: {}, recebido: {}.",
            expectedBodySize, packet.body.size());
        session->close();
        return;
    }

    // Recria a struct completa a partir do header e do body
    PacketLoginUser loginPacket;
    loginPacket.header = packet.header;
    std::memcpy(
        // Destino: o endereço da struct APÓS o campo do cabeçalho
        reinterpret_cast<uint8_t*>(&loginPacket) + sizeof(PacketHeader),
        // Origem: o buffer do corpo do pacote
        packet.body.data(),
        // Tamanho: o tamanho esperado do corpo
        expectedBodySize
    );

    // Envia a requisição completa para a fila de processamento do AccountService
    m_accountService.queueLoginRequest(session, loginPacket);
}