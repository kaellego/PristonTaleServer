# Priston Tale Server

Servidor de jogo em C++20, baseado na refatoração do código legado do Priston Tale. O projeto separa rede, serviços de aplicação, runtime do jogo e infraestrutura de persistência.

> Status: base funcional para testes de login, personagens, inventário, warehouse, movimentação, combate simplificado e NPCs de fallback. Antes de produção, ainda é obrigatório corrigir a persistência completa do personagem, autenticar o handoff para o GameServer e concluir os layouts legados.

## Estado atual e riscos prioritários

O servidor já inicializa, autentica contas, transfere sessões entre as portas
`5055` e `5056`, carrega personagens, executa movimentação/combate básico e
valida operações de itens. Isso ainda não representa uma implementação completa
do cliente original.

Os riscos que devem ser tratados antes de adicionar novos sistemas são:

1. `GameWorld` atualmente monta snapshots parciais durante movimento,
   regeneração e desconexão. Persistir esses snapshots pode sobrescrever classe,
   nível, atributos, ouro, equipamentos e regeneração.
2. O handoff para a porta `5056` ainda usa associação por IP em parte do fluxo.
   O ticket precisa ser um token assinado, com expiração e nonce validados no
   GameServer; calcular uma assinatura e descartá-la não autentica a sessão.
3. Falhas de leitura do SQL, blob inválido e estado inexistente ainda precisam
   ser estados distintos. Um erro temporário do banco não deve fazer o servidor
   voltar automaticamente para `.chr`/`.dat` antigos.
4. Inventário ou warehouse SQL vazios devem continuar sendo estados válidos; a
   ausência de itens não pode disparar fallback para arquivos legados.
5. Criação e exclusão ainda alteram `CharacterInfo`, `PlayerStateProto` e os
   arquivos legados em etapas separadas. É necessário concluir a transação SQL e
   a política de backup/limpeza antes de considerar o fluxo atômico.

O log `0 NPCs carregados do GameDB` significa que o servidor está usando os NPCs
de fallback do mapa 0. Spawns reais só devem ser habilitados depois da importação
de coordenadas válidas e da instalação dos modelos no cliente.

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
- carregamento do catálogo de itens a partir de `ItemList`;
- núcleo de NPCs: registro por mapa, envio legado `OPCODE_ADD_NPC` e clique validado por sessão, serial e distância;
- catálogo de 123 definições e 105 associações de mapa importáveis de `ZeroPT-Server\GameServer\NPC`/`Field`;
- carregamento de NPCs habilitados a partir de `GameDB.dbo.NpcList` e `GameDB.dbo.NpcSpawn`, aceitando várias instâncias do mesmo NPC.

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

### Estado de jogador no SQL/Protobuf

O estado canônico de personagem, inventário e warehouse agora é salvo em
`UserDB.dbo.PlayerStateProto`. O payload usa o wire format do Protobuf definido
em [`database/player_state.proto`](database/player_state.proto) e é salvo
diretamente como `VARBINARY(MAX)`. Registros antigos em chunks/Base64 continuam
sendo lidos como versão 1 e são convertidos para o formato binário na próxima
gravação. O codec oficial é gerado por `protoc` a partir de
[`database/player_state.proto`](database/player_state.proto) e os arquivos
gerados ficam versionados em `src/Core/GameLogic/Generated`.

Cada blob versão 2 recebe `StateMac`, um HMAC-SHA-256 calculado sobre
`StateType`, `OwnerKey` e o payload Protobuf. A chave vem exclusivamente de
`ZEROPT_ITEM_SECRET` e deve ter pelo menos 32 caracteres; ela não deve ser
gravada no Git nem compartilhada com o cliente.

Os arquivos `.chr` e `.dat` são usados somente como fonte de migração legada
quando ainda não existe estado SQL. Depois da migração, novas gravações vão
somente para SQL; os arquivos antigos não são apagados automaticamente para
permitir backup e recuperação. Exclusão explícita de personagem remove também
o estado legado correspondente.

Inventário e warehouse são gravados em uma única transação SQL quando a
operação altera os dois lados. O cliente nunca fornece autoridade sobre serial,
checksum, proprietário, quantidade ou dinheiro: esses campos são normalizados
e validados no servidor antes da persistência.

Protobuf é apenas serialização, não criptografia nem autenticação. A proteção
contra cliente adulterado depende de validação server-side, segredo externo de
integridade dos itens, nonce/sequence do protocolo e controle de acesso ao SQL.
O `DatabasePool` mantém conexões físicas reutilizáveis por banco e limita o
acesso concorrente por fila. Registros versão 1 sem MAC continuam aceitos apenas
para migração; novas gravações e versões 2 sem MAC válido são rejeitadas.

O servidor usa SQL Server por ODBC/SOCI. As tabelas iniciais ficam em [`database/init.sql`](database/init.sql). A configuração de conexão fica em `server.ini` e não deve conter credenciais reais versionadas.

A implementação atual combina:

- banco de dados para contas, vínculos, catálogo de itens e estado Protobuf;
- arquivos legados somente como origem de migração e backup;
- `PlayerRepository` para carregar o estado SQL e migrar arquivos ainda não importados.

### Catálogo e posicionamento de NPCs

Os backups em `ZeroPT-DataBase\DataBase` não possuem `NPCList` nem `MapNPC`. O catálogo disponível está nos arquivos legados do servidor. Para importá-lo:

```powershell
.\database\import-npc.ps1
```

O script é idempotente e cria `NpcList`/`NpcSpawn`. Sem um CSV de coordenadas `MapNPC`, as instâncias ficam desabilitadas deliberadamente; isso evita espalhar NPCs em coordenadas inventadas. Para habilitar spawns reais, use `-MapNpcCsv` com as colunas `SourceMap,LegacyFile,MapId,X,Y,Z,AngleY,Enabled`.

O estado SQL de inventário e personagem já possui transações próprias, mas o
fluxo completo ainda não é atômico com `CharacterInfo` nem com a limpeza dos
arquivos legados. Uma falha entre essas etapas pode exigir reconciliação manual.
Além disso, `GameWorld` precisa persistir o estado completo, e não snapshots
parciais. A estrutura `Item` e parte de `CharacterData` ainda não representam
todos os campos do cliente original; quests, skills, pets, timers e progressos
avançados continuam pendentes.

## Compilação

Pré-requisitos:

- Visual Studio 2022 com C++20;
- MSBuild;
- Boost.Asio e dependências configuradas;
- ODBC Driver 18 for SQL Server, ou driver compatível configurado;
- banco de dados criado e inicializado com `database/init.sql`;
- Protobuf instalado pelo vcpkg; o repositório inclui `vcpkg.json`.

No PowerShell, a compilação Debug x64 pode ser feita com:

```powershell
.\build.ps1 -Configuration Debug -Platform x64
```

Se a dependência ainda não estiver instalada:

```powershell
C:\vcpkg\vcpkg.exe install --x-install-root=vcpkg-installed --triplet x64-windows
```

Defina o segredo antes de iniciar o servidor:

```powershell
$env:ZEROPT_ITEM_SECRET = 'substitua-por-um-segredo-com-32-caracteres-ou-mais'
```

O executável é gerado em `x64\Debug\GameServer.exe`.

## Testes disponíveis

- `tests\ProtocolSmoke.ps1`: fluxo básico de protocolo, login e sessão;
- `tests\LoadSmoke.ps1`: teste de múltiplas conexões;
- `tests\StartServer.ps1`: inicialização rápida do servidor;
- `tests\StartServerAndClientDebug.ps1`: execução com coleta de diagnóstico do cliente.

Ainda não existe um teste de integração completo que valide, com o cliente original, criação das 11 classes, seleção de personagem, inventário, combate, warehouse e encerramento simultâneo de todas as conexões.

## Pendências do roadmap

- [ ] Persistir o `CharacterData` completo em movimento, regeneração e desconexão;
- [ ] Diferenciar SQL indisponível, blob inválido e estado inexistente antes de usar arquivos legados;
- [ ] Impedir fallback de inventário/warehouse SQL quando o estado válido estiver vazio;
- [ ] Usar ticket HMAC com nonce e expiração no handoff LoginServer → GameServer;
- [ ] Tornar `CharacterInfo`, `PlayerStateProto` e a política de arquivos legados consistentes em criação/exclusão;
- [ ] Mapear todos os layouts reais do protocolo legado em `smTRANS_COMMAND`;
- [ ] Implementar respostas legadas completas para login, servidores e personagens;
- [ ] Corrigir e validar os pacotes legados de dano `ATTACK_DAMAGE`, `DISP_DAMAGE` e `ATTACK_RESULT`;
- [ ] Implementar skills e efeitos persistidos;
- [x] Implementar o núcleo de NPCs e interação segura;
- [x] Importar o catálogo de NPCs e carregar spawns habilitados do `GameDB`;
- [ ] Implementar lojas, NPCs com eventos e drops de NPC;
- [ ] Implementar quests e progresso de quests;
- [ ] Implementar party, raid, trade e comunicação de clã;
- [ ] Implementar eventos, coin shop e comunicação entre servidores;
- [ ] Completar os campos de `Item`, `CharacterData`, warehouse e inventário;
- [ ] Tornar criação, exclusão e salvamento de personagem transacionais;
- [ ] Importar e validar os metadados reais de classe, nível e slot de todos os itens;
- [ ] Adicionar suporte visual das classes 9–11 ao cliente e instalar os assets corretos;
- [ ] Executar testes end-to-end com o cliente legado e testes de carga prolongados.

O teste de protocolo existente não substitui o teste com o cliente original. O
fluxo end-to-end ainda precisa cobrir login, seleção de servidor, seleção e
exclusão de personagem, reconexão após reinício, NPCs, itens, warehouse,
combate, desconexão durante gravação, replay e 100 ou mais conexões simultâneas.

## Segurança e operação

Não desative a validação do `LegacyPacketCodec`, checksum, contador, expiração de ticket ou limites de tamanho para contornar incompatibilidades do cliente. Quando um pacote legado ainda não possui layout confirmado, ele deve ser rejeitado ou encaminhado para um handler explicitamente seguro, nunca aceito como autenticação válida.

Antes de executar em produção:

1. altere as credenciais padrão do banco;
2. valide os assets e o endereço/porta do cliente;
3. execute os testes de protocolo e carga;
4. revise os logs e o encerramento controlado do processo;
5. faça backup dos arquivos `.chr`, inventário e warehouse antes de qualquer migração.
