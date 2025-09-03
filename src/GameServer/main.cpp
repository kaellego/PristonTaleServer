// GameServer/main.cpp

#include <iostream>
#include <exception>
#include <memory>

// ESTA É A LINHA CHAVE:
// Ela "puxa" a declaração da classe Application da nossa biblioteca Core
// para dentro do executável, permitindo que a usemos.
#include "Application.h"

int main() {
    try {
        std::cout << "Inicializando Servidor Priston Tale..." << std::endl;

        // Cria a instância principal da aplicação.
        auto app = std::make_unique<Application>();

        std::cout << "[main] Servidor inicializado com sucesso. Executando..." << std::endl;

        // Inicia o servidor.
        app->run();

    }
    catch (const std::exception& e) {
        std::cerr << "\n!! ERRO FATAL !!\n" << e.what() << std::endl;
        std::cin.get();
        return 1;
    }

    std::cout << "[main] Servidor encerrado de forma limpa." << std::endl;
    return 0;
}