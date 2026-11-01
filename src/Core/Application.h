#pragma once

#include <memory>
#include <boost/asio/io_context.hpp>

// --- InclusÃµes Completas dos ServiÃ§os ---
#include "Config/ServerConfig.h"
#include "Shared/GlobalState.h"
#include "Logging/LogService.h"
#include "Database/DatabasePool.h"
#include "GameLogic/ItemRepository.h"
#include "GameLogic/ItemPolicy.h"
#include "GameLogic/PlayerRepository.h"
#include "Network/PacketDispatcher.h"
#include "Network/Server.h"

// Adicionamos os novos serviÃ§os aqui
#include "GameLogic/Services/AccountService.h"
#include "GameLogic/Services/CharacterService.h"
#include "GameLogic/Services/UserService.h"
#include "GameLogic/Services/InventoryService.h"
#include "GameLogic/EventQueue.h"
#include "GameLogic/GameWorld.h"

class Application {
public:
    Application();
    void run();

private:
    void buildServices();
    void mainLoop();

    // O contexto de I/O do Boost.Asio
    std::shared_ptr<boost::asio::io_context> m_io_context;

    // --- ServiÃ§os do Servidor ---
    std::unique_ptr<ServerConfig> m_config;
    std::unique_ptr<GlobalState> m_globalState;
    std::unique_ptr<LogService> m_logService;
    std::unique_ptr<DatabasePool> m_dbPool;
    std::unique_ptr<ItemRepository> m_itemRepository;
    std::unique_ptr<ItemPolicy> m_itemPolicy;
    std::unique_ptr<PlayerRepository> m_playerRepository;

    // Nossos novos serviÃ§os de lÃ³gica de jogo
    std::unique_ptr<CharacterService> m_characterService;
    std::unique_ptr<UserService> m_userService;
    std::unique_ptr<AccountService> m_accountService;
    std::unique_ptr<InventoryService> m_inventoryService;
    EventQueue m_gameEvents;
    GameWorld m_gameWorld;

    std::unique_ptr<PacketDispatcher> m_packetDispatcher;
    std::unique_ptr<Server> m_networkServer;

    bool m_isRunning;
};
