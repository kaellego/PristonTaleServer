#include "Application.h"

#include "Logging/LogService.h"
#include "GameLogic/Services/AccountService.h"
#include "GameLogic/Services/CharacterService.h"
#include "GameLogic/Services/UserService.h"

#include <iostream>
#include <thread>
#include <chrono>

Application::Application() : m_isRunning(false) {
    // A criaÃƒÆ’Ã‚Â§ÃƒÆ’Ã‚Â£o do LogService precisa vir antes de qualquer chamada a ele.
    // ÃƒÆ’Ã¢â‚¬Â° uma boa prÃƒÆ’Ã‚Â¡tica inicializar o GlobalState primeiro, pois outros podem depender dele.
    m_globalState = std::make_unique<GlobalState>();
    m_logService = std::make_unique<LogService>("ServerLog.txt", *m_globalState);

    m_logService->info("Inicializando a aplicacao do servidor...");

    try {
        buildServices();
        m_logService->info("Todos os servicos foram construidos com sucesso.");
    }
    catch (const std::exception& e) {
        int fase = 1;
        // A primeira {} serÃƒÆ’Ã‚Â¡ substituÃƒÆ’Ã‚Â­da por 'fase', a segunda por e.what()
        m_logService->error("Erro na fase {}: {}", fase, e.what());

        // Re-lanÃƒÆ’Ã‚Â§a a exceÃƒÆ’Ã‚Â§ÃƒÆ’Ã‚Â£o para que o main() possa capturÃƒÆ’Ã‚Â¡-la e encerrar
        // de forma limpa, permitindo que o usuÃƒÆ’Ã‚Â¡rio veja a mensagem no console.
        throw;
    }
}

void Application::buildServices() {
    // --- ETAPA 1: ServiÃƒÆ’Ã‚Â§os Base (sem dependÃƒÆ’Ã‚Âªncias complexas) ---
    m_config = std::make_unique<ServerConfig>("server.ini");

    //m_logService->info("Versao do Jogo: {}", m_config->getGameVersion());
    //m_logService->info("Chave XOR Calculada: 0x{:02x}", m_config->getXorKey());

    m_globalState->gameVersion = m_config->getGameVersion();
    m_globalState->isGameServer = m_config->getThisServerInfo().isGameServer;
    m_globalState->isLoginServer = m_config->getThisServerInfo().isLoginServer;

    // --- ETAPA 2: ServiÃƒÆ’Ã‚Â§os de Dados ---
    m_dbPool = std::make_unique<DatabasePool>(*m_config, *m_logService);
    m_playerRepository = std::make_unique<PlayerRepository>("./Data"); // Supondo que a pasta Data estÃƒÆ’Ã‚Â¡ no diretÃƒÆ’Ã‚Â³rio do executÃƒÆ’Ã‚Â¡vel
    m_itemRepository = std::make_unique<ItemRepository>(*m_dbPool, *m_logService);

    // --- ETAPA 3: ServiÃƒÆ’Ã‚Â§os de LÃƒÆ’Ã‚Â³gica de Jogo ---
    m_characterService = std::make_unique<CharacterService>(*m_dbPool, *m_playerRepository, *m_logService);
    m_userService = std::make_unique<UserService>(*m_dbPool, *m_logService);
    m_accountService = std::make_unique<AccountService>(*m_dbPool, *m_characterService, *m_userService, *m_logService, *m_config);
    m_inventoryService = std::make_unique<InventoryService>(*m_playerRepository, *m_characterService);


    // --- ETAPA 4: Rede e Despacho de Pacotes ---
    m_io_context = std::make_shared<boost::asio::io_context>();
    
    m_packetDispatcher = std::make_unique<PacketDispatcher>(
        *m_globalState,
        *m_playerRepository,
        *m_itemRepository,
        *m_accountService,
        *m_logService,
        *m_userService,
        *m_characterService,
        *m_inventoryService,
        m_gameWorld
    );

    const auto& serverInfo = m_config->getThisServerInfo();
    uint8_t gameXorKey = m_config->getXorKey();

    m_networkServer = std::make_unique<Server>(m_io_context, serverInfo.port, *m_packetDispatcher, *m_logService, gameXorKey);
}

void Application::run() {
    m_isRunning = true;

    // --- Inicia as Threads de Rede ---
    // Cria um pool de threads para processar as operaÃƒÆ’Ã‚Â§ÃƒÆ’Ã‚Âµes de I/O de rede.
    const auto thread_count = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> network_threads;
    network_threads.reserve(thread_count);

    m_logService->info("Iniciando {} threads de rede...", thread_count);
    for (unsigned int i = 0; i < thread_count; ++i) {
        network_threads.emplace_back([this]() {
            m_io_context->run();
            });
    }

    m_logService->info("Servidor de rede em execucao. Iniciando loop principal do jogo.");

    // --- Inicia o Loop Principal do Jogo (Game Loop) ---
    // Este loop roda na thread principal e ÃƒÆ’Ã‚Â© responsÃƒÆ’Ã‚Â¡vel pelos ticks de lÃƒÆ’Ã‚Â³gica.
    mainLoop();

    // --- Encerramento ---
    m_logService->info("Aguardando as threads de rede finalizarem...");
    for (auto& t : network_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void Application::mainLoop() {
    using clock = std::chrono::steady_clock;
    auto next_tick = clock::now();
    const auto tick_interval = std::chrono::milliseconds(50); // 20 ticks por segundo

    while (m_isRunning) {
        m_gameEvents.process();
        m_gameWorld.update();
        // Para encerrar o servidor de forma limpa, vocÃƒÆ’Ã‚Âª precisaria de um mecanismo
        // para definir m_isRunning como false (ex: um comando no console).

        // Atualiza o tick count global (se estiver usando a funÃƒÆ’Ã‚Â§ÃƒÆ’Ã‚Â£o do Windows)
        // m_globalState->tickCount = GetTickCount(); 

        // --- LÃƒÆ’Ã‚Â³gica do Jogo executada a cada tick ---
        // Aqui vocÃƒÆ’Ã‚Âª chamaria os mÃƒÆ’Ã‚Â©todos de update dos seus outros serviÃƒÆ’Ã‚Â§os
        // Ex:
        // m_unitServer->update();
        // m_mapServer->update();

        // Dorme atÃƒÆ’Ã‚Â© o prÃƒÆ’Ã‚Â³ximo tick para controlar o FPS do servidor
        next_tick += tick_interval;
        std::this_thread::sleep_until(next_tick);
    }

    // Pede ao serviÃƒÆ’Ã‚Â§o de I/O para parar quando o loop terminar
    m_io_context->stop();
}
