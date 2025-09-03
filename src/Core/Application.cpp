#include "Application.h"

#include <iostream>
#include <thread>
#include <chrono>

Application::Application() : m_isRunning(false) {
    std::cout << "Inicializando a aplicacao do servidor..." << std::endl;
    try {
        buildServices();
        std::cout << "Todos os servicos foram construidos com sucesso." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "ERRO FATAL DURANTE A INICIALIZACAO: " << e.what() << std::endl;
        // Re-lança a exceção para que o main() possa capturá-la e encerrar.
        throw;
    }
}

void Application::buildServices() {
    // 1. Configuração e Estado Global (não têm dependências)
    m_config = std::make_unique<ServerConfig>("server.ini");
    m_globalState = std::make_unique<GlobalState>();
    m_globalState->isGameServer = m_config->getThisServerInfo().isGameServer;
    m_globalState->isLoginServer = m_config->getThisServerInfo().isLoginServer;

    // 2. Serviços de Dados (dependem da configuração)
    m_dbManager = std::make_unique<DatabaseManager>(*m_config);
    m_playerRepository = std::make_unique<PlayerRepository>("./Data"); // Supondo que a pasta Data está no diretório do executável

    // 3. Serviços de Lógica de Jogo (dependem dos serviços de dados)
    m_itemRepository = std::make_unique<ItemRepository>(*m_dbManager);

    // 4. Rede e Despacho de Pacotes (dependem de tudo)
    m_io_context = std::make_shared<boost::asio::io_context>();

    // O PacketDispatcher precisa de acesso a outros serviços para manipular os pacotes
    m_packetDispatcher = std::make_unique<PacketDispatcher>(*m_globalState, *m_playerRepository, *m_itemRepository);

    // O Servidor de Rede precisa do io_context e do dispatcher
    const auto& serverInfo = m_config->getThisServerInfo();
    m_networkServer = std::make_unique<Server>(m_io_context, serverInfo.port, *m_packetDispatcher);
}

void Application::run() {
    m_isRunning = true;

    // --- Inicia as Threads de Rede ---
    // Cria um pool de threads para processar as operações de I/O de rede.
    const auto thread_count = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> network_threads;
    network_threads.reserve(thread_count);

    std::cout << "Iniciando " << thread_count << " threads de rede..." << std::endl;
    for (unsigned int i = 0; i < thread_count; ++i) {
        network_threads.emplace_back([this]() {
            m_io_context->run();
            });
    }

    std::cout << "Servidor de rede em execucao. Iniciando loop principal do jogo." << std::endl;

    // --- Inicia o Loop Principal do Jogo (Game Loop) ---
    // Este loop roda na thread principal e é responsável pelos ticks de lógica.
    mainLoop();

    // --- Encerramento ---
    std::cout << "Aguardando as threads de rede finalizarem..." << std::endl;
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