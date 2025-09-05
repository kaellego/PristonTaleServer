#include "GameLogic/Handlers/LoginHandler.h"
#include "GameLogic/Services/AccountService.h"
#include "Network/ClientSession.h"
#include "Shared/datatypes.h"
#include "Shared/GlobalState.h"
#include "Logging/LogService.h"
#include "Shared/Constants.h"
#include "Logging/LogEvents.h"
#include <cstring> // Para std::memcpy

LoginHandler::LoginHandler(AccountService& accountService, LogService& logService, GlobalState& globalState)
    : m_accountService(accountService),
    m_logService(logService),
    m_globalState(globalState)
{}

void LoginHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    // 1. Ignora o pacote se for um Game Server
    if (m_globalState.isGameServer) {
        return;
    }

    // 2. Validação de tamanho
    constexpr size_t expectedBodySize = sizeof(PacketLoginUser) - sizeof(PacketHeader);
    if (packet.body.size() != expectedBodySize) {
        m_logService.warn("Pacote de login recebido com tamanho de corpo invalido. Fechando conexao.");
        session->close();
        return;
    }

    // 3. Reconstrói a struct do pacote
    PacketLoginUser loginPacket;
    loginPacket.header = packet.header;
    std::memcpy(
        reinterpret_cast<uint8_t*>(&loginPacket) + sizeof(PacketHeader),
        packet.body.data(),
        expectedBodySize
    );

    // 4. Validação da versão do jogo
    if (loginPacket.iVersion != m_globalState.gameVersion) {
        m_logService.warn("Tentativa de login com versao incorreta do jogo. Cliente: {}, Servidor: {}. Enviando erro.",
            loginPacket.iVersion, m_globalState.gameVersion);

        // Monta e envia o pacote de versão incorreta
        PacketAccountLoginCode responseStruct{};
        responseStruct.header.opcode = static_cast<uint32_t>(Opcodes::AccountLoginCode);
        responseStruct.iCode = static_cast<int>(Log::LoginResult::WrongVersion); // Supondo que WrongVersion exista
        strncpy_s(responseStruct.szMessage, "Game version is incorrect please update", sizeof(responseStruct.szMessage) - 1);

        const size_t bodySize = sizeof(PacketAccountLoginCode) - sizeof(PacketHeader);
        const uint8_t* bodyPtr = reinterpret_cast<const uint8_t*>(&responseStruct) + sizeof(PacketHeader);
        std::vector<uint8_t> body(bodyPtr, bodyPtr + bodySize);

        Packet response(responseStruct.header.opcode, body);
        session->send(response);
        return;
    }

    // 5. Se todas as validações passaram, incrementa os contadores e enfileira a requisição
    m_globalState.loginAttempts++;
    // TODO: session->getUserData()->iVerifiedLogin++; (quando UserData for implementado)

    // TODO: Chamar o CheatService
    // m_cheatService.onLoginUserPacket(session, loginPacket);

    m_accountService.queueLoginRequest(session, loginPacket);
}