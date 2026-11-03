#include "Application.h"

#include "Logging/LogService.h"
#include "GameLogic/Services/AccountService.h"
#include "GameLogic/Services/CharacterService.h"
#include "GameLogic/Services/UserService.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <boost/asio/signal_set.hpp>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <windows.h>
#include "GameLogic/ItemIntegrity.h"

namespace {
    std::filesystem::path runtimeFile(const char* name) {
        char modulePath[MAX_PATH]{};
        const DWORD length = GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
        if (length == 0 || length >= MAX_PATH) return std::filesystem::path(name);
        return std::filesystem::path(modulePath).parent_path() / name;
    }
}

Application::Application() : m_isRunning(false)
{
    // A criaÃ§Ã£o do LogService precisa vir antes de qualquer chamada a ele.
    // Ã‰ uma boa prÃ¡tica inicializar o GlobalState primeiro, pois outros podem depender dele.
    m_globalState = std::make_unique<GlobalState>();
    m_logService = std::make_unique<LogService>(runtimeFile("ServerLog.txt").string(), *m_globalState);

    m_logService->info("Inicializando a aplicacao do servidor...");

    try
    {
        buildServices();
        m_logService->info("Todos os servicos foram construidos com sucesso.");
    }
    catch (const std::exception &e)
    {
        int fase = 1;
        // A primeira {} serÃ¡ substituÃ­da por 'fase', a segunda por e.what().
        m_logService->error("Erro na fase {}: {}", fase, e.what());

        // RelanÃ§a a exceÃ§Ã£o para que o main() possa capturÃ¡-la e encerrar
        // de forma limpa, permitindo que o usuÃ¡rio veja a mensagem no console.
        throw;
    }
}

void Application::buildServices()
{
    // --- ETAPA 1: ServiÃ§os base (sem dependÃªncias complexas) ---
    m_config = std::make_unique<ServerConfig>(runtimeFile("server.ini").string());

    // m_logService->info("Versao do Jogo: {}", m_config->getGameVersion());
    // m_logService->info("Chave XOR Calculada: 0x{:02x}", m_config->getXorKey());

    m_globalState->gameVersion = m_config->getGameVersion();
    m_globalState->isGameServer = m_config->getThisServerInfo().isGameServer;
    m_globalState->isLoginServer = m_config->getThisServerInfo().isLoginServer;

    char* itemSecret = nullptr;
    size_t itemSecretLength = 0;
    _dupenv_s(&itemSecret, &itemSecretLength, "ZEROPT_ITEM_SECRET");
    if (!itemSecret || itemSecretLength < 32)
        throw std::runtime_error("ZEROPT_ITEM_SECRET deve conter pelo menos 32 caracteres.");
    ItemIntegrity::setSecret(itemSecret);
    free(itemSecret);

    // --- ETAPA 2: ServiÃ§os de dados ---
    m_dbPool = std::make_unique<DatabasePool>(*m_config, *m_logService);
    m_playerRepository = std::make_unique<PlayerRepository>(runtimeFile("Data").string(), m_dbPool.get());
    m_gameWorld.setPersistenceCallback([this](const std::string& name, CharacterData data) {
        // GameWorld owns the complete session snapshot. Saving it directly
        // prevents movement/regen from rebuilding a zeroed character record.
        if (data.szName[0] == '\0')
            std::memcpy(data.szName, name.c_str(), std::min(name.size(), sizeof(data.szName) - 1));
        if (!m_playerRepository->saveCharacter(data))
            m_logService->error("Falha ao persistir o personagem '{}' apos movimento/desconexao.", name);
    });
    m_npcRepository = std::make_unique<NpcRepository>(*m_dbPool, *m_logService);
    m_npcShopRepository = std::make_unique<NpcShopRepository>(*m_dbPool);
    const auto loadedNpcs = m_npcRepository->loadEnabledSpawns(m_gameWorld);
    if (loadedNpcs == 0) {
        // Fallback only until GameDB receives real MapNPC coordinates.
        const bool first = m_gameWorld.registerNpc(NpcEntity{3, 0x81000003u, 0, {0.0f, 0.0f, 0.0f}, 0,
                                          "AcaciaShop", "char\\npc\\TN-005\\TN-005.ase", 0,
                                          100, 100, 0, 0, false, true});
        const bool second = m_gameWorld.registerNpc(NpcEntity{10, 0x8100000Au, 0, {3072.0f, 0.0f, 0.0f}, 0,
                                          "CaveKeeper", "char\\npc\\TN-001\\TN-001.ase", 0,
                                          100, 100, 0, 0, false, true});
        m_logService->info("Fallback de NPCs registrado no mapa 0: {}", (first ? 1 : 0) + (second ? 1 : 0));
    }
    m_itemRepository = std::make_unique<ItemRepository>(*m_dbPool, *m_logService);
    m_itemPolicy = std::make_unique<ItemPolicy>(*m_dbPool);

    // --- ETAPA 3: ServiÃ§o de lÃ³gica de jogo ---
    m_characterService = std::make_unique<CharacterService>(*m_dbPool, *m_playerRepository, *m_logService);
    m_userService = std::make_unique<UserService>(*m_dbPool, *m_logService);
    m_accountService = std::make_unique<AccountService>(*m_dbPool, *m_characterService, *m_userService, *m_logService, *m_config);
    m_inventorySqlRepository = std::make_unique<InventorySqlRepository>(*m_dbPool);
    m_inventoryService = std::make_unique<InventoryService>(*m_playerRepository, *m_characterService, *m_itemPolicy, m_inventorySqlRepository.get());

    // --- ETAPA 4: Rede e Despacho de Pacotes ---
    m_io_context = std::make_shared<boost::asio::io_context>();
    m_signals = std::make_unique<boost::asio::signal_set>(*m_io_context, SIGINT, SIGTERM);
    m_signals->async_wait([this](const boost::system::error_code& error, int) {
        if (!error) requestShutdown();
    });

    m_packetDispatcher = std::make_unique<PacketDispatcher>(
        *m_globalState,
        *m_playerRepository,
        *m_itemRepository,
        *m_accountService,
        *m_logService,
        *m_userService,
        *m_characterService,
        *m_inventoryService,
        m_gameWorld,
        *m_npcRepository,
        *m_npcShopRepository);

    const auto &serverInfo = m_config->getThisServerInfo();
    uint8_t gameXorKey = m_config->getXorKey();

    m_networkServer = std::make_unique<Server>(m_io_context, serverInfo.port, *m_packetDispatcher, *m_logService, gameXorKey);

    // A login process must also accept the GameServer endpoint advertised to
    // the legacy client, otherwise server selection can never complete.
    if (serverInfo.isLoginServer) {
        for (const auto& gameServer : m_config->getAllServers()) {
            if (gameServer.isGameServer && gameServer.port != serverInfo.port) {
                m_gameNetworkServer = std::make_unique<Server>(
                    m_io_context, gameServer.port, *m_packetDispatcher, *m_logService, gameXorKey, true);
                break;
            }
        }
    }
}

void Application::run()
{
    m_isRunning = true;

    // --- Inicia as Threads de Rede ---
    // Cria um pool de threads para processar as operaÃ§Ãµes de I/O de rede.
    const auto thread_count = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> network_threads;
    network_threads.reserve(thread_count);

    m_logService->info("Iniciando {} threads de rede...", thread_count);
    for (unsigned int i = 0; i < thread_count; ++i)
    {
        network_threads.emplace_back([this]()
                                     { m_io_context->run(); });
    }

    m_logService->info("Servidor de rede em execucao. Iniciando loop principal do jogo.");

    // --- Inicia o Loop Principal do Jogo (Game Loop) ---
    // Este loop roda na thread principal e Ã© responsÃ¡vel pelos ticks de lÃ³gica.
    mainLoop();

    requestShutdown();

    // --- Encerramento ---
    m_logService->info("Aguardando as threads de rede finalizarem...");
    for (auto &t : network_threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

void Application::requestShutdown()
{
    if (!m_isRunning.exchange(false)) return;
    m_logService->info("SolicitaÃ§Ã£o de encerramento recebida; iniciando shutdown gracioso.");
    if (m_networkServer) m_networkServer->stop();
    if (m_gameNetworkServer) m_gameNetworkServer->stop();
    if (m_signals) m_signals->cancel();
    if (m_io_context) m_io_context->stop();
}

void Application::mainLoop()
{
    using clock = std::chrono::steady_clock;
    auto next_tick = clock::now();
    const auto tick_interval = std::chrono::milliseconds(50); // 20 ticks por segundo

    while (m_isRunning)
    {
        m_gameEvents.process();
        m_gameWorld.update();
        // Para encerrar o servidor de forma limpa, vocÃª precisaria de um mecanismo
        // para definir m_isRunning como false (ex: um comando no console).

        // Atualiza o tick count global (se estiver usando a funÃ§Ã£o do Windows)
        // m_globalState->tickCount = GetTickCount();

        // --- Logica do Jogo executada a cada tick ---
        // Aqui vocÃª chamaria os mÃ©todos de update dos seus outros serviÃ§os de lÃ³gica, como o GameWorld, UnitServer, MapServer, etc.
        // Ex:
        // m_unitServer->update();
        // m_mapServer->update();

        // Dorme atÃ© o proximo tick para controlar o FPS do servidor
        next_tick += tick_interval;
        std::this_thread::sleep_until(next_tick);
    }

    requestShutdown();
}

