#include "Application.h"

#include "Logging/LogService.h"
#include "GameLogic/Services/AccountService.h"
#include "GameLogic/Services/CharacterService.h"
#include "GameLogic/Services/UserService.h"

#include <iostream>
#include <thread>
#include <chrono>

Application::Application() : m_isRunning(false) {
    std::cout << "[INFO] Inicializando a aplicacao do servidor..." << std::endl;
    try {
        buildServices();
        std::cout << "[INFO] Todos os servicos foram construidos com sucesso." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "ERRO FATAL DURANTE A INICIALIZACAO: " << e.what() << std::endl;
        // Re-lança a exceção para que o main() possa capturá-la e encerrar.
        throw;
    }
}

void Application::buildServices() {
    // --- ETAPA 1: Serviços Base (sem dependências complexas) ---
    m_config = std::make_unique<ServerConfig>("server.ini");
    m_globalState = std::make_unique<GlobalState>();
    m_logService = std::make_unique<LogService>("ServerLog.txt", *m_globalState);

    m_logService->info("[INFO] Versao do Jogo: {}", m_config->getGameVersion());
    m_logService->info("[INFO] Chave XOR Calculada: 0x{:02x}", m_config->getXorKey());

    m_globalState->isGameServer = m_config->getThisServerInfo().isGameServer;
    m_globalState->isLoginServer = m_config->getThisServerInfo().isLoginServer;

    // --- ETAPA 2: Serviços de Dados ---
    m_dbManager = std::make_unique<DatabaseManager>(*m_config, *m_logService);
    m_playerRepository = std::make_unique<PlayerRepository>("./Data"); // Supondo que a pasta Data está no diretório do executável
    m_itemRepository = std::make_unique<ItemRepository>(*m_dbManager);

    // --- ETAPA 3: Serviços de Lógica de Jogo ---
    m_characterService = std::make_unique<CharacterService>(*m_dbManager, *m_playerRepository, *m_logService);
    m_userService = std::make_unique<UserService>(*m_logService);
    m_accountService = std::make_unique<AccountService>(*m_dbManager, *m_characterService, *m_userService, *m_logService);

    // --- ETAPA 4: Rede e Despacho de Pacotes ---
    m_io_context = std::make_shared<boost::asio::io_context>();
    m_packetDispatcher = std::make_unique<PacketDispatcher>(*m_globalState, *m_playerRepository, *m_itemRepository, *m_accountService, *m_logService);

    const auto& serverInfo = m_config->getThisServerInfo();
    uint8_t gameXorKey = m_config->getXorKey();

    m_networkServer = std::make_unique<Server>(m_io_context, serverInfo.port, *m_packetDispatcher, *m_logService, gameXorKey);
}

void Application::run() {
    m_isRunning = true;

    // --- Inicia as Threads de Rede ---
    // Cria um pool de threads para processar as operações de I/O de rede.
    const auto thread_count = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> network_threads;
    network_threads.reserve(thread_count);

    std::cout << "[INFO] Iniciando " << thread_count << " threads de rede..." << std::endl;
    for (unsigned int i = 0; i < thread_count; ++i) {
        network_threads.emplace_back([this]() {
            m_io_context->run();
            });
    }

    std::cout << "[INFO] Servidor de rede em execucao. Iniciando loop principal do jogo." << std::endl;

    // --- Inicia o Loop Principal do Jogo (Game Loop) ---
    // Este loop roda na thread principal e é responsável pelos ticks de lógica.
    mainLoop();

    // --- Encerramento ---
    std::cout << "[INFO] Aguardando as threads de rede finalizarem..." << std::endl;
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
        // Para encerrar o servidor de forma limpa, você precisaria de um mecanismo
        // para definir m_isRunning como false (ex: um comando no console).

        // Atualiza o tick count global (se estiver usando a função do Windows)
        // m_globalState->tickCount = GetTickCount(); 

        // --- Lógica do Jogo executada a cada tick ---
        // Aqui você chamaria os métodos de update dos seus outros serviços
        // Ex:
        // m_unitServer->update();
        // m_mapServer->update();

        // Dorme até o próximo tick para controlar o FPS do servidor
        next_tick += tick_interval;
        std::this_thread::sleep_until(next_tick);
    }

    // Pede ao serviço de I/O para parar quando o loop terminar
    m_io_context->stop();
}