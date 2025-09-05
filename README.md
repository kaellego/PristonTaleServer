# Priston Tale - Servidor Refatorado (Base de Projeto)

![C++](https://img.shields.io/badge/C++-20-blue.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

Este repositório contém a base para um servidor de Priston Tale, resultado de um esforço de refatoração de um código-fonte legado. O objetivo principal é transformar a base de código original, escrita em um estilo C mais antigo, em uma aplicação C++ moderna, robusta, segura e de fácil manutenção.

O foco não é apenas fazer o servidor funcionar, mas construí-lo sobre uma fundação de boas práticas de engenharia de software e Padrões de Design (`Design Patterns`).

> ⚠️ **Status do Projeto:** Em Desenvolvimento Ativo.
> A base da arquitetura, incluindo a camada de rede assíncrona, criptografia, configuração e o sistema de banco de dados, está implementada e funcional. A implementação dos handlers de pacotes e da lógica de jogo está em andamento.

## Filosofia e Objetivos

A reestruturação deste projeto segue três pilares principais:

* **Modularidade e Desacoplamento:** Isolar as responsabilidades em serviços independentes (`AccountService`, `LogService`, etc.) para que as alterações em uma parte do sistema não afetem outras.
* **Segurança e Robustez:** Utilizar recursos modernos do C++ (Smart Pointers, RAII, `enum class`) para eliminar classes inteiras de bugs, como vazamentos de memória, corrupção de dados e falhas inesperadas.
* **Performance e Escalabilidade:** Construir a camada de rede com Boost.Asio, utilizando um modelo assíncrono e multithread para lidar com um grande número de conexões simultâneas de forma eficiente.

## Arquitetura e Padrões de Projeto

A nova arquitetura foi construída utilizando vários Padrões de Design fundamentais para garantir um código limpo e escalável:

### 1. Injeção de Dependência (Dependency Injection)
O padrão mais importante da nossa arquitetura. Em vez de serviços criarem suas próprias dependências (o que gera forte acoplamento), as dependências são "injetadas" de fora. A classe `Application` atua como nosso "Contêiner de Injeção de Dependência", criando todos os serviços e passando-os para os construtores dos serviços que deles precisam.

### 2. Strategy
A lógica de manipulação de pacotes é implementada com o padrão Strategy. O `PacketDispatcher` é o "Contexto" que recebe um pacote. Ele não sabe o que fazer com o pacote, apenas consulta um mapa de "Estratégias" (`IPacketHandler`). Cada `Opcode` é mapeado a uma classe de handler concreta (`LoginHandler`, `PingHandler`, etc.), que contém a lógica específica para aquele pacote. Isso torna a adição de novos pacotes trivial, sem a necessidade de modificar `switch-cases` gigantes.

### 3. RAII (Resource Acquisition Is Initialization)
Um princípio fundamental do C++ moderno. Todos os recursos (memória, conexões de banco de dados, arquivos, threads) são gerenciados por objetos. A aquisição do recurso acontece no construtor do objeto, e a liberação acontece automaticamente no destrutor. Usamos isso extensivamente com:
* `std::unique_ptr` para gerenciar o ciclo de vida dos nossos serviços.
* `std::lock_guard` para garantir que `mutexes` sejam sempre liberados.
* Nossa classe `PooledConnection` para garantir que as conexões de banco de dados sejam sempre devolvidas ao pool.

### 4. Pool de Conexões (Connection Pool)
Para evitar o alto custo de abrir e fechar conexões com o banco de dados a cada consulta, a classe `DatabasePool` mantém um conjunto de conexões prontas para uso. A lógica é thread-safe, utilizando `std::mutex` e `std::condition_variable` para que múltiplas threads possam "emprestar" e "devolver" conexões de forma segura e eficiente.

### 5. Asynchronous Design (Proactor)
A camada de rede é construída com **Boost.Asio**, seguindo o padrão Proactor. As operações de I/O (leitura e escrita de rede) são iniciadas e não bloqueiam a execução. Quando uma operação é concluída, uma função de "callback" (um `lambda`) é chamada para processar o resultado. Isso permite que um pequeno número de threads gerencie milhares de conexões de clientes simultaneamente.

## Estrutura do Projeto
A solução no Visual Studio está dividida em:
* **`Core` (Biblioteca Estática - `.lib`):** Contém toda a lógica, serviços, rede e classes do servidor. É o "motor" do projeto.
* **`GameServer` (Executável - `.exe`):** Contém apenas a função `main()`. Sua única responsabilidade é criar uma instância da classe `Application` (do `Core`) e executá-la.

## Como Compilar

### Pré-requisitos
* Visual Studio 2022 (com toolset para C++20).
* Biblioteca Boost (versão 1.76 ou superior).

### Passos para Compilação
1.  Clone este repositório.
2.  Compile as bibliotecas do Boost necessárias (`system`, `thread`, etc.) executando `bootstrap.bat` e `b2.exe` na pasta do Boost.
3.  Abra o arquivo `.sln` no Visual Studio.
4.  Configure as propriedades do projeto para apontar para os diretórios de `include` e `lib` do Boost.
5.  Compile a solução.

## Próximos Passos (Roadmap)
- [x] Estabelecer arquitetura base com Injeção de Dependência.
- [x] Implementar camada de rede assíncrona com Boost.Asio.
- [x] Implementar o sistema de criptografia de duas camadas.
- [x] Implementar o Pool de Conexões de Banco de Dados.
- [x] Implementar o fluxo de login completo.
- [ ] Implementar o handler de seleção de personagens.
- [ ] Refatorar e implementar os serviços `CharacterService` e `UserService` com lógica de jogo.
- [ ] Adicionar handlers para todos os outros pacotes do jogo.
