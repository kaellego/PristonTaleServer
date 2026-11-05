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

O workspace contém tipos de domínio de mapa/NPC, configuração INI e um servidor TCP assíncrono com handshake HMAC-SHA-256. O servidor devolve frames autenticados apenas para validar o transporte; isso não é ainda o protocolo legado nem o protocolo de produção. O encoder legado continua vazio de propósito e não deve ser preenchido com um layout presumido.

Quando Rust estiver instalado, execute na raiz desta pasta:

```powershell
cargo test
cargo run
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
