# Rust server rewrite

Esta pasta é a reimplementação incremental do servidor em Rust. O servidor C++ legado continua na raiz e não é alterado por este crate.

## Princípios

- domínio sem dependência de pacote legado;
- validação antes de qualquer conversão de transporte;
- nenhum `unsafe` no núcleo inicial;
- persistência atrás de interfaces;
- testes unitários para regras antes de implementar o protocolo;
- compatibilidade legada adicionada somente com layouts documentados.

## Estado atual

O workspace contém tipos de domínio de mapa/NPC, configuração INI, conexão opcional com SQL Server e um servidor TCP assíncrono com handshake HMAC-SHA-256. O servidor devolve frames autenticados apenas para validar o transporte; isso não é ainda o protocolo legado nem o protocolo de produção. O encoder legado continua vazio de propósito e não deve ser preenchido com um layout presumido.

Quando Rust estiver instalado, execute na raiz desta pasta:

```powershell
cargo test
cargo run
```

Ou use os scripts PowerShell:

```powershell
.\scripts\build-server.ps1
.\scripts\start-server.ps1
```

Defina antes de iniciar:

```powershell
$env:ZEROPT_SESSION_SECRET = 'use-um-segredo-com-pelo-menos-32-caracteres'
```

O executável lê `../server.ini` quando iniciado a partir de `rust-server`. Se o arquivo não for encontrado, usa a porta padrão `5055`. O frame interno experimental possui:

```text
u32 frame_size_le   // opcode + payload, entre 4 e 1 MiB
u32 opcode_le
bytes payload
```

Esse formato é temporário e não deve ser confundido com um pacote do cliente legado.

O handshake exige um frame `opcode=1` com nonce de 32 bytes, seguido de `opcode=2` com HMAC-SHA-256 do nonce. O servidor só aceita frames de aplicação depois de responder `opcode=3`. O cliente legado não fala esse protocolo.

## SQL Server

A conexão é opcional durante o scaffold e usa somente variáveis externas:

```powershell
$env:ZEROPT_SQL_HOST = '127.0.0.1'
$env:ZEROPT_SQL_PORT = '1433'
$env:ZEROPT_SQL_DATABASE = 'UserDB'
$env:ZEROPT_SQL_USER = 'sa'
$env:ZEROPT_SQL_PASSWORD = 'senha-fora-do-repositorio'
$env:ZEROPT_SQL_TRUST_CERT = '1'
```

Quando `ZEROPT_SQL_HOST` estiver definido, o servidor valida a conexão com `SELECT 1` antes de abrir o listener. Ainda não existem repositórios de contas/personagens; essa conexão não altera dados.

## Arquitetura atual

O fluxo usa `LoginService`, `MapService` e `ShopService`, repositórios SQL e um pool SQL Server compartilhado. O Tokio usa o runtime multithread; cada cliente é atendido por uma task assíncrona e `MaxUsers` limita as sessões simultâneas. O pool usa até `min(MaxUsers, 32)` conexões por padrão e pode ser ajustado com `ZEROPT_SQL_POOL_SIZE` entre 1 e 256. O login exige conta e personagem pertencente à conta; a entrada no mapa carrega NPCs do banco e a compra usa transação SQL.

SeaORM não foi adicionado. Tiberius permanece para manter controle sobre SQL Server, transações e layouts legados; ORM será reavaliado após testes reais de carga e integração.
