#pragma once

#include <string>
#include <cstdint>
#include <atomic> // Para contadores de usuários thread-safe

/**
 * @struct GlobalState
 * @brief Armazena o estado global compartilhado do servidor de forma segura.
 *
 * Substitui os ponteiros de memória fixos e variáveis globais por um
 * objeto de estado coeso que pode ser passado para os serviços que
 * precisam dele.
 */
struct GlobalState {
    // std::atomic garante que incrementos e decrementos sejam seguros
    // em um ambiente com múltiplas threads.
    std::atomic<uint32_t> usersOnline{ 0 };
    std::atomic<uint32_t> unitsOnline{ 0 };
    std::atomic<uint32_t> loginAttempts{ 0 };
    int gameVersion = 0;

    // O tick count pode ser atualizado pelo loop principal do servidor.
    // Não precisa ser atômico se for atualizado apenas por uma thread.
    uint32_t tickCount = 0;

    std::string serverName;
    bool isGameServer = false;
    bool isLoginServer = false;
};