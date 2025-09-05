#include "GameLogic/Handlers/PingHandler.h"
#include "Network/ClientSession.h"
#include "Shared/datatypes.h"
#include "Shared/Constants.h"
#include "Logging/LogService.h"
#include "GameLogic/Services/UserService.h"
#include "Shared/GlobalState.h"
#include <iostream>

PingHandler::PingHandler(LogService& logService, UserService& userService, GlobalState& globalState)
    : m_logService(logService), m_userService(userService), m_globalState(globalState)
{}

void PingHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    // 1. Validação do tamanho do pacote
    constexpr size_t expectedBodySize = sizeof(PacketPing) - sizeof(PacketHeader);
    if (packet.body.size() != expectedBodySize) {
        m_logService.warn("Pacote de Ping recebido com tamanho de corpo invalido. Esperado: {}, recebido: {}.",
            expectedBodySize, packet.body.size());
        session->close();
        return;
    }

    // 2. Reconstrói a struct do pacote de forma segura
    PacketPing pingPacket;
    pingPacket.header = packet.header;
    std::memcpy(
        reinterpret_cast<uint8_t*>(&pingPacket) + sizeof(PacketHeader),
        packet.body.data(),
        expectedBodySize
    );

    // 3. Lógica do "Pong": envia o pacote de volta com o timestamp do servidor
    // É uma boa prática criar um novo pacote de resposta.
    PacketPing pongPacket = pingPacket;
    pongPacket.header.opcode = static_cast<uint32_t>(Opcodes::Ping); // Garante o opcode correto
    pongPacket.dwTick = GetTickCount(); // Atualiza com o tempo do servidor

    Packet response(pongPacket.header,
        std::vector<uint8_t>(
            reinterpret_cast<uint8_t*>(&pongPacket) + sizeof(PacketHeader),
            reinterpret_cast<uint8_t*>(&pongPacket) + sizeof(PacketPing)
        )
    );
    session->send(response);

    m_logService.packet("-> Pacote Recebido: Ping | Respondido com Pong.");

    // 4. Lógica de "Notice Counter" (substituída por um placeholder)
    // if (session->getNoticeCounter() < SOME_GLOBAL_NOTICE_COUNT) {
    //     m_noticeService.showNotice(session);
    // }
    // A chamada original `CALL_WITH_ARG1(0x00550930, ...)` seria movida para um serviço apropriado.

    // 5. Lógica Anti-Cheat
    if (m_globalState.isLoginServer && pingPacket.CheatEngineDetected && session->isAuthenticated()) {
        m_logService.warn("!! DETECCAO DE CHEAT !! Conta: {}, Personagem: {}. Causa: CheatEngine Detectado via Ping.",
            session->getAccountName(), "PersonagemAqui"); // session->getCharacterName()

        // TODO: A lógica de banimento seria chamada aqui, por exemplo:
        // m_userService.banAccount(session->getAccountName(), "AUTOMATIC - CheatEngine detected");
    }
}