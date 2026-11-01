# Priston Tale Server

Servidor de jogo em C++20, baseado na refatoração do código legado do Priston Tale. O projeto separa rede, serviços de aplicação, runtime do jogo e infraestrutura de persistência.

> Status: base funcional para testes de login, personagens, inventário, warehouse, movimentação e combate simplificado. O protocolo legado e os sistemas avançados do jogo ainda estão em evolução.

## Arquitetura

```text
Network Layer
  ClientSession, Server, PacketDispatcher, LegacyGatewayProtocol

Application Services
  AccountService, CharacterService, InventoryService, UserService

Game Runtime
  GameSession, GameWorld, EventQueue

Infrastructure
  DatabasePool, Orm, repositories, LogService, ServerConfig
```

As responsabilidades principais são:

- I/O de rede assíncrono com Boost.Asio;
- processamento de login em fila própria;
- validação de sessão, ticket, expiração, rate limit e proteção contra replay;
- lógica do jogo no `GameWorld`;
- acesso ao banco por ODBC/SOCI e persistência complementar em arquivos legados.

## Funcionalidades implementadas

- Inicialização e encerramento controlado do servidor;
- sessões assíncronas de clientes e despacho por opcode;
- login com validação de hash, banimento, limite por IP e ticket de sessão;
- seleção de servidor e gateway para o protocolo legado;
- seleção, criação e exclusão de personagens;
- migração de personagens legados para o formato atual de `.chr`;
- suporte no servidor às classes de código `1..11`;
- inventário, warehouse, movimentação básica de itens e uso/equipamento com `ItemPolicy`;
- movimentação básica de jogadores com validação de entidade, distância e células próximas;
- combate básico com validação de atacante, alvo, alcance e dano;
- carregamento inicial do personagem, posição, atributos, vida, mana, ouro e equipamentos;
- carregamento do catálogo de itens a partir de `ItemList`.

Os handlers atualmente registrados cobrem keep-alive, login, ping, personagens, inventário/warehouse, movimentação e combate moderno. Isso não significa que todos os opcodes declarados no legado estejam implementados.

## Classes de personagem

O código do servidor usa a ordem numérica legada abaixo:

| Código | Classe | Corpo padrão | Rosto padrão |
|---:|---|---|---|
| 1 | Fighter | `b001.ini` | `tmh-b01.inf` |
| 2 | Mechanician | `a001.ini` | `tmh-a01.inf` |
| 3 | Archer | `d001.ini` | `tfh-D01.inf` |
| 4 | Pikeman | `c001.ini` | `tmh-c01.inf` |
| 5 | Atalanta | `mb001.ini` | `Mfh-B01.inf` |
| 6 | Knight | `ma001.ini` | `Mmh-A01.inf` |
| 7 | Magician | `md001.ini` | `Mmh-D01.inf` |
| 8 | Priestess | `mc001.ini` | `Mfh-C01.inf` |
| 9 | Assassin | `e001.ini` | `tfh-e01.inf` |
| 10 | Shaman | `me001.ini` | `Mmh-e01.inf` |
| 11 | Martial Artist | `f001.ini` | `tfh-f01.inf` |

Essa tabela é aplicada no `CharacterHandler`, `AccountService` e `LegacyGatewayProtocol`. A migração e a criação também aceitam o intervalo `1..11`.

### Limitação do cliente legado

O `SrcGame` original ainda declara `MAX_JOBNUM = 8` e possui somente oito opções de criação na interface. Portanto, o servidor aceita as classes 9, 10 e 11, mas o cliente original não as torna selecionáveis sem alterações adicionais na interface e nos dados do cliente.

Na cópia atual de `ZeroPT-Client`, os arquivos de corpo `.ini` usados pelos nomes legados não foram encontrados; há arquivos `.inx` correspondentes para parte dos modelos. O asset `tfh-f01.inf`, usado pela classe 11, também não foi encontrado. É necessário instalar o pacote correto de assets antes de validar a renderização das 11 classes.

## Banco de dados e persistência

O servidor usa SQL Server por ODBC/SOCI. As tabelas iniciais ficam em [`database/init.sql`](database/init.sql). A configuração de conexão fica em `server.ini` e não deve conter credenciais reais versionadas.

A implementação atual combina:

- banco de dados para contas, vínculos de personagens e catálogo de itens;
- arquivos legados para personagens, inventário e warehouse;
- `PlayerRepository` para carregar, salvar e migrar esses arquivos.

Essa combinação ainda não é uma transação única. Uma falha entre a gravação no banco e a gravação do arquivo pode exigir reconciliação manual. A estrutura `Item` e parte de `CharacterData` ainda não representam todos os campos do cliente original; quests, skills, pets, timers e progressos avançados continuam pendentes.

## Compilação

Pré-requisitos:

- Visual Studio 2022 com C++20;
- MSBuild;
- Boost.Asio e dependências configuradas;
- ODBC Driver 18 for SQL Server, ou driver compatível configurado;
- banco de dados criado e inicializado com `database/init.sql`.

No PowerShell, a compilação Debug x64 pode ser feita com:

```powershell
.\build.ps1 -Configuration Debug -Platform x64
```

O executável é gerado em `x64\Debug\GameServer.exe`.

## Testes disponíveis

- `tests\ProtocolSmoke.ps1`: fluxo básico de protocolo, login e sessão;
- `tests\LoadSmoke.ps1`: teste de múltiplas conexões;
- `tests\StartServer.ps1`: inicialização rápida do servidor;
- `tests\StartServerAndClientDebug.ps1`: execução com coleta de diagnóstico do cliente.

Ainda não existe um teste de integração completo que valide, com o cliente original, criação das 11 classes, seleção de personagem, inventário, combate, warehouse e encerramento simultâneo de todas as conexões.

## Pendências do roadmap

- [ ] Mapear todos os layouts reais do protocolo legado em `smTRANS_COMMAND`;
- [ ] Implementar respostas legadas completas para login, servidores e personagens;
- [ ] Corrigir e validar os pacotes legados de dano `ATTACK_DAMAGE`, `DISP_DAMAGE` e `ATTACK_RESULT`;
- [ ] Implementar skills e efeitos persistidos;
- [ ] Implementar NPCs, lojas e drops;
- [ ] Implementar quests e progresso de quests;
- [ ] Implementar party, raid, trade e comunicação de clã;
- [ ] Implementar eventos, coin shop e comunicação entre servidores;
- [ ] Completar os campos de `Item`, `CharacterData`, warehouse e inventário;
- [ ] Tornar criação, exclusão e salvamento de personagem transacionais;
- [ ] Importar e validar os metadados reais de classe, nível e slot de todos os itens;
- [ ] Adicionar suporte visual das classes 9–11 ao cliente e instalar os assets corretos;
- [ ] Executar testes end-to-end com o cliente legado e testes de carga prolongados.

## Segurança e operação

Não desative a validação do `LegacyPacketCodec`, checksum, contador, expiração de ticket ou limites de tamanho para contornar incompatibilidades do cliente. Quando um pacote legado ainda não possui layout confirmado, ele deve ser rejeitado ou encaminhado para um handler explicitamente seguro, nunca aceito como autenticação válida.

Antes de executar em produção:

1. altere as credenciais padrão do banco;
2. valide os assets e o endereço/porta do cliente;
3. execute os testes de protocolo e carga;
4. revise os logs e o encerramento controlado do processo;
5. faça backup dos arquivos `.chr`, inventário e warehouse antes de qualquer migração.
