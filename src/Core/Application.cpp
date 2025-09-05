#include "Application.h"

#include "Logging/LogService.h"
#include "GameLogic/Services/AccountService.h"
#include "GameLogic/Services/CharacterService.h"
#include "GameLogic/Services/UserService.h"

#include <iostream>
#include <thread>
#include <chrono>

Application::Application() : m_isRunning(false) {
    // A cria��o do LogService precisa vir antes de qualquer chamada a ele.
    // � uma boa pr�tica inicializar o GlobalState primeiro, pois outros podem depender dele.
    m_globalState = std::make_unique<GlobalState>();
    m_logService = std::make_unique<LogService>("ServerLog.txt", *m_globalState);

    m_logService->info("Inicializando a aplicacao do servidor...");

    try {
        buildServices();
        m_logService->info("Todos os servicos foram construidos com sucesso.");
    }
    catch (const std::exception& e) {
        int fase = 1;
        // A primeira {} ser� substitu�da por 'fase', a segunda por e.what()
        m_logService->error("Erro na fase {}: {}", fase, e.what());

        // Re-lan�a a exce��o para que o main() possa captur�-la e encerrar
        // de forma limpa, permitindo que o usu�rio veja a mensagem no console.
        throw;
    }
}

void Application::buildServices() {
    // --- ETAPA 1: Servi�os Base (sem depend�ncias complexas) ---
    m_config = std::make_unique<ServerConfig>("server.ini");

    m_logService->info("Versao do Jogo: {}", m_config->getGameVersion());
    m_logService->info("Chave XOR Calculada: 0x{:02x}", m_config->getXorKey());

    m_globalState->isGameServer = m_config->getThisServerInfo().isGameServer;
    m_globalState->isLoginServer = m_config->getThisServerInfo().isLoginServer;

    // --- ETAPA 2: Servi�os de Dados ---
    m_dbPool = std::make_unique<DatabasePool>(*m_config, *m_logService);
    m_playerRepository = std::make_unique<PlayerRepository>("./Data"); // Supondo que a pasta Data est� no diret�rio do execut�vel
    m_itemRepository = std::make_unique<ItemRepository>(*m_dbPool, *m_logService);

    // --- ETAPA 3: Servi�os de L�gica de Jogo ---
    m_characterService = std::make_unique<CharacterService>(*m_dbPool, *m_playerRepository, *m_logService);
    m_userService = std::make_unique<UserService>(*m_logService);
    m_accountService = std::make_unique<AccountService>(*m_dbPool, *m_characterService, *m_userService, *m_logService);

    // --- ETAPA 4: Rede e Despacho de Pacotes ---
    m_io_context = std::make_shared<boost::asio::io_context>();
    
    m_packetDispatcher = std::make_unique<PacketDispatcher>(
        *m_globalState,
        *m_playerRepository,
        *m_itemRepository,
        *m_accountService,
        *m_logService,
        *m_userService
    );

    const auto& serverInfo = m_config->getThisServerInfo();
    uint8_t gameXorKey = m_config->getXorKey();

    m_networkServer = std::make_unique<Server>(m_io_context, serverInfo.port, *m_packetDispatcher, *m_logService, gameXorKey);
}

void Application::run() {
    m_isRunning = true;

    // --- Inicia as Threads de Rede ---
    // Cria um pool de threads para processar as opera��es de I/O de rede.
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
    // Este loop roda na thread principal e � respons�vel pelos ticks de l�gica.
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
        // Para encerrar o servidor de forma limpa, voc� precisaria de um mecanismo
        // para definir m_isRunning como false (ex: um comando no console).

        // Atualiza o tick count global (se estiver usando a fun��o do Windows)
        // m_globalState->tickCount = GetTickCount(); 

        // --- L�gica do Jogo executada a cada tick ---
        // Aqui voc� chamaria os m�todos de update dos seus outros servi�os
        // Ex:
        // m_unitServer->update();
        // m_mapServer->update();

        // Dorme at� o pr�ximo tick para controlar o FPS do servidor
        next_tick += tick_interval;
        std::this_thread::sleep_until(next_tick);
    }

    // Pede ao servi�o de I/O para parar quando o loop terminar
    m_io_context->stop();
}