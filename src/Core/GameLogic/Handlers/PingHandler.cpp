#include "GameLogic/Handlers/PingHandler.h"
#include "Network/ClientSession.h"
#include "Shared/datatypes.h"
#include "Shared/Constants.h"
#include "Logging/LogService.h"
#include "GameLogic/Services/UserService.h" // Necessário para a lógica de ban
#include "Shared/GlobalState.h"

PingHandler::PingHandler(LogService& logService, UserService& userService, GlobalState& globalState)
    : m_logService(logService), m_userService(userService), m_globalState(globalState)
{}

void PingHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    // 1. Validação de tamanho
    constexpr size_t expectedBodySize = sizeof(PacketPing) - sizeof(PacketHeader);
    if (packet.body.size() != expectedBodySize) {
        return; // Ignora pacotes de ping malformados silenciosamente
    }

    // 2. Reconstrói a struct do pacote de ping recebido
    PacketPing receivedPingPacket;
    receivedPingPacket.header = packet.header;
    std::memcpy(
        reinterpret_cast<uint8_t*>(&receivedPingPacket) + sizeof(PacketHeader),
        packet.body.data(),
        expectedBodySize
    );

    // 3. Lógica do "Pong" (Eco)
    //    Simplesmente enviamos o mesmo pacote que recebemos de volta para o cliente.
    //    O cliente usará o campo 'dwTime' original para calcular a latência.
    session->send(packet);

    m_logService.packet("-> Pacote Ping recebido de {}. Respondido com Pong.", session->getAccountName());

    // 4. Lógica Anti-Cheat (traduzida do código original)
    if (m_globalState.isLoginServer && receivedPingPacket.CheatEngineDetected && session->isAuthenticated()) {
        m_logService.warn("!! DETECCAO DE CHEAT !! Conta: {}. Causa: CheatEngine Detectado via Ping.",
            session->getAccountName());

        // No futuro, o UserService terá um método para banir a conta
        // m_userService.banAccount(session->getAccountName(), "AUTOMATIC - CheatEngine detected");
    }
}