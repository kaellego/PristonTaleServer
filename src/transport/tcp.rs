use std::{io, net::SocketAddr};

use crate::auth::{verify, NONCE_SIZE};
use crate::infrastructure::repositories::SqlRepositories;
use crate::transport::legacy::encode_npc_info;
use crate::{
    application::services::{LoginService, MapService, ShopService},
    domain::MapId,
};
use deadpool_tiberius::Pool;
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpListener, TcpStream},
};

const HEADER_SIZE: usize = 8;
const MAX_FRAME_SIZE: usize = 1024 * 1024;
const OPCODE_AUTHENTICATE: u32 = 2;
const OPCODE_AUTH_OK: u32 = 3;
const OPCODE_ACCOUNT_LOGIN: u32 = 10;
const OPCODE_MAP_ENTER: u32 = 11;
const OPCODE_LOGIN_OK: u32 = 12;
const OPCODE_CHARACTER_LIST: u32 = 13;
const OPCODE_NPC_INFO: u32 = 20;
const OPCODE_SHOP_BUY: u32 = 30;

#[derive(Debug, PartialEq, Eq)]
pub struct Frame {
    pub opcode: u32,
    pub payload: Vec<u8>,
}

pub struct TcpServer {
    listener: TcpListener,
    sql: Option<Pool>,
}

impl TcpServer {
    pub async fn bind(
        address: impl tokio::net::ToSocketAddrs,
        sql: Option<Pool>,
    ) -> io::Result<Self> {
        Ok(Self {
            listener: TcpListener::bind(address).await?,
            sql,
        })
    }

    pub fn local_addr(&self) -> io::Result<SocketAddr> {
        self.listener.local_addr()
    }

    pub async fn run(self) -> io::Result<()> {
        let secret = std::env::var("ZEROPT_SESSION_SECRET").map_err(|_| {
            io::Error::new(io::ErrorKind::InvalidInput, "ZEROPT_SESSION_SECRET ausente")
        })?;
        if secret.len() < 32 {
            return Err(io::Error::new(
                io::ErrorKind::InvalidInput,
                "ZEROPT_SESSION_SECRET deve ter 32 caracteres",
            ));
        }
        loop {
            let (stream, peer) = self.listener.accept().await?;
            let secret = secret.clone();
            let sql = self.sql.clone();
            tokio::spawn(async move {
                if let Err(error) = handle_connection(stream, secret.as_bytes(), sql).await {
                    eprintln!("TCP connection {peer} closed: {error}");
                }
            });
        }
    }
}

async fn handle_connection(
    mut stream: TcpStream,
    secret: &[u8],
    sql: Option<Pool>,
) -> io::Result<()> {
    let hello = read_frame(&mut stream)
        .await?
        .ok_or_else(|| io::Error::new(io::ErrorKind::UnexpectedEof, "handshake ausente"))?;
    if hello.opcode != 1 || hello.payload.len() != NONCE_SIZE {
        return Err(io::Error::new(
            io::ErrorKind::PermissionDenied,
            "hello inválido",
        ));
    }
    let nonce: [u8; NONCE_SIZE] = hello.payload.try_into().unwrap();
    let proof = read_frame(&mut stream)
        .await?
        .ok_or_else(|| io::Error::new(io::ErrorKind::UnexpectedEof, "prova ausente"))?;
    if proof.opcode != OPCODE_AUTHENTICATE || !verify(secret, &nonce, &proof.payload) {
        return Err(io::Error::new(
            io::ErrorKind::PermissionDenied,
            "autenticação inválida",
        ));
    }
    stream
        .write_all(&encode_frame(&Frame {
            opcode: OPCODE_AUTH_OK,
            payload: vec![],
        }))
        .await?;
    if let Some(pool) = sql {
        let login = read_frame(&mut stream)
            .await?
            .ok_or_else(|| io::Error::new(io::ErrorKind::UnexpectedEof, "conta ausente"))?;
        if login.opcode != OPCODE_ACCOUNT_LOGIN
            || login.payload.is_empty()
            || login.payload.len() > 32
        {
            return Err(io::Error::new(
                io::ErrorKind::PermissionDenied,
                "login RustPT invalido",
            ));
        }
        let separator = login
            .payload
            .iter()
            .position(|byte| *byte == 0)
            .ok_or_else(|| {
                io::Error::new(
                    io::ErrorKind::InvalidData,
                    "login deve conter conta e personagem",
                )
            })?;
        let (account_bytes, character_bytes) =
            (&login.payload[..separator], &login.payload[separator + 1..]);
        let account_name = std::str::from_utf8(account_bytes)
            .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "nome de conta invalido"))?;
        let character_name = std::str::from_utf8(character_bytes).map_err(|_| {
            io::Error::new(io::ErrorKind::InvalidData, "nome de personagem invalido")
        })?;
        let repo = SqlRepositories::new(pool.clone());
        let login_service = LoginService::new(repo.clone());
        let account = login_service
            .account(account_name)
            .await
            .map_err(io::Error::other)?
            .ok_or_else(|| {
                io::Error::new(io::ErrorKind::PermissionDenied, "conta nao encontrada")
            })?;
        if account.banned {
            return Err(io::Error::new(
                io::ErrorKind::PermissionDenied,
                "conta bloqueada",
            ));
        }
        if !login_service
            .character_is_owned(account_name, character_name)
            .await
            .map_err(io::Error::other)?
        {
            return Err(io::Error::new(
                io::ErrorKind::PermissionDenied,
                "personagem nao pertence a conta",
            ));
        }
        stream
            .write_all(&encode_frame(&Frame {
                opcode: OPCODE_LOGIN_OK,
                payload: account.id.0.to_le_bytes().to_vec(),
            }))
            .await?;
        let characters = login_service
            .characters(account_name)
            .await
            .map_err(io::Error::other)?;
        let mut character_payload = Vec::new();
        for character in characters {
            character_payload.extend_from_slice(&(character.name.len() as u16).to_le_bytes());
            character_payload.extend_from_slice(character.name.as_bytes());
            character_payload.extend_from_slice(&character.experience.to_le_bytes());
        }
        stream
            .write_all(&encode_frame(&Frame {
                opcode: OPCODE_CHARACTER_LIST,
                payload: character_payload,
            }))
            .await?;
        while let Some(frame) = read_frame(&mut stream).await? {
            if frame.opcode == OPCODE_SHOP_BUY && frame.payload.len() == 12 {
                let npc_id = i32::from_le_bytes(frame.payload[0..4].try_into().unwrap());
                let item_code = i32::from_le_bytes(frame.payload[4..8].try_into().unwrap());
                let quantity = i32::from_le_bytes(frame.payload[8..12].try_into().unwrap());
                let result = ShopService::new(repo.clone())
                    .buy(
                        character_name,
                        crate::domain::NpcId(npc_id),
                        item_code,
                        quantity,
                    )
                    .await;
                let status = if result.is_ok() { 1i32 } else { 0i32 };
                stream
                    .write_all(&encode_frame(&Frame {
                        opcode: OPCODE_SHOP_BUY,
                        payload: status.to_le_bytes().to_vec(),
                    }))
                    .await?;
                continue;
            }
            if frame.opcode == OPCODE_MAP_ENTER && frame.payload.len() == 4 {
                let map = MapId(i32::from_le_bytes(frame.payload.try_into().unwrap()));
                for npc in MapService::new(repo.clone())
                    .npcs(map)
                    .await
                    .map_err(io::Error::other)?
                {
                    if let Some(payload) =
                        encode_npc_info(&npc, 0x8100_0000u32.wrapping_add(npc.id.0 as u32))
                    {
                        stream
                            .write_all(&encode_frame(&Frame {
                                opcode: OPCODE_NPC_INFO,
                                payload,
                            }))
                            .await?;
                    }
                }
                continue;
            }
            if frame.opcode == 0 {
                return Err(io::Error::new(io::ErrorKind::InvalidData, "opcode zero"));
            }
            stream.write_all(&encode_frame(&frame)).await?;
        }
        return Ok(());
    }
    while let Some(frame) = read_frame(&mut stream).await? {
        // Transport only validates framing. Domain dispatch is intentionally separate.
        if frame.opcode == 0 {
            return Err(io::Error::new(io::ErrorKind::InvalidData, "opcode zero"));
        }
        stream.write_all(&encode_frame(&frame)).await?;
    }
    Ok(())
}

async fn read_frame(stream: &mut TcpStream) -> io::Result<Option<Frame>> {
    let mut header = [0u8; HEADER_SIZE];
    match stream.read_exact(&mut header).await {
        Ok(_) => {}
        Err(error) if error.kind() == io::ErrorKind::UnexpectedEof => return Ok(None),
        Err(error) => return Err(error),
    }

    let frame_size = u32::from_le_bytes(header[0..4].try_into().unwrap()) as usize;
    let opcode = u32::from_le_bytes(header[4..8].try_into().unwrap());
    if !(4..=MAX_FRAME_SIZE).contains(&frame_size) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "invalid frame size",
        ));
    }

    let mut payload = vec![0u8; frame_size - 4];
    stream.read_exact(&mut payload).await?;
    Ok(Some(Frame { opcode, payload }))
}

fn encode_frame(frame: &Frame) -> Vec<u8> {
    let frame_size = (4 + frame.payload.len()) as u32;
    let mut bytes = Vec::with_capacity(HEADER_SIZE + frame.payload.len());
    bytes.extend_from_slice(&frame_size.to_le_bytes());
    bytes.extend_from_slice(&frame.opcode.to_le_bytes());
    bytes.extend_from_slice(&frame.payload);
    bytes
}

#[cfg(test)]
mod tests {
    use super::{encode_frame, Frame, HEADER_SIZE, MAX_FRAME_SIZE};

    #[test]
    fn encodes_frame_with_explicit_little_endian_header() {
        let frame = Frame {
            opcode: 7,
            payload: vec![1, 2, 3],
        };
        let bytes = encode_frame(&frame);
        assert_eq!(bytes.len(), HEADER_SIZE + 3);
        assert_eq!(&bytes[0..4], &7u32.to_le_bytes());
        assert_eq!(&bytes[4..8], &7u32.to_le_bytes());
        assert!(MAX_FRAME_SIZE >= bytes.len());
    }
}
