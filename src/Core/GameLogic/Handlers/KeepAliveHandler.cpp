#include "GameLogic/Handlers/KeepAliveHandler.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"
#include "Shared/Constants.h"

KeepAliveHandler::KeepAliveHandler(LogService &logService) : m_logService(logService) {}

void KeepAliveHandler::handle(std::shared_ptr<ClientSession> session, const Packet &packet)
{
    try
    {
        m_logService.packet("Pacote Keep-Alive (0x0000) recebido do cliente {}", session->socket().remote_endpoint().address().to_string());
    }
    catch (...)
    {
    }
    if (packet.header.opcode == static_cast<uint32_t>(Opcodes::LegacyConnected))
        session->send(Packet(static_cast<uint32_t>(Opcodes::LegacyConnected)));
    // Normalmente, nenhuma resposta é necessária para um keep-alive.
}
