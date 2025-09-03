#pragma once

#include <memory>
#include <boost/asio/io_context.hpp>

// --- Inclusões Completas dos Serviços ---
#include "Config/ServerConfig.h"
#include "Shared/GlobalState.h"
#include "Database/DatabaseManager.h"
#include "GameLogic/ItemRepository.h"
#include "GameLogic/PlayerRepository.h"
#include "GameLogic/Services/AccountService.h"
#include "Network/PacketDispatcher.h"
#include "Network/Server.h"

class Application {
public:
    Application();
    void run();

private:
    void buildServices();
    void mainLoop();

    // O contexto de I/O do Boost.Asio
    std::shared_ptr<boost::asio::io_context> m_io_context;

    // --- Serviços do Servidor ---
    std::unique_ptr<ServerConfig> m_config;
    std::unique_ptr<GlobalState> m_globalState;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ItemRepository> m_itemRepository;
    std::unique_ptr<PlayerRepository> m_playerRepository;
    std::unique_ptr<PacketDispatcher> m_packetDispatcher;
    std::unique_ptr<AccountService> m_accountService;
    std::unique_ptr<Server> m_networkServer;

    bool m_isRunning;
};