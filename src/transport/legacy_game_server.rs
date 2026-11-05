use std::io;

use deadpool_tiberius::Pool;
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpListener, TcpStream},
    sync::broadcast,
};

use super::legacy::{encode_character_record, encode_play_data_group};
use super::ticket::TicketStore;
use crate::{
    application::services::{LoginService, MapService},
    infrastructure::repositories::SqlRepositories,
};

pub struct LegacyGameServer {
    listener: TcpListener,
    pool: Option<Pool>,
    tickets: TicketStore,
}

impl LegacyGameServer {
    pub async fn bind(
        address: impl tokio::net::ToSocketAddrs,
        pool: Option<Pool>,
        tickets: TicketStore,
    ) -> io::Result<Self> {
        Ok(Self {
            listener: TcpListener::bind(address).await?,
            pool,
            tickets,
        })
    }

    pub async fn run(self) -> io::Result<()> {
        let (sessions, _) = broadcast::channel::<Vec<u8>>(256);
        loop {
            let (stream, peer) = self.listener.accept().await?;
            let pool = self.pool.clone();
            let tickets = self.tickets.clone();
            let sessions = sessions.clone();
            tokio::spawn(async move {
                if let Err(error) =
                    handle(stream, peer.ip().to_string(), pool, tickets, sessions).await
                {
                    tracing::debug!(%error, "legacy game connection closed");
                }
            });
        }
    }
}

async fn handle(
    mut stream: TcpStream,
    address: String,
    pool: Option<Pool>,
    tickets: TicketStore,
    sessions: broadcast::Sender<Vec<u8>>,
) -> io::Result<()> {
    stream.write_all(&command(13)).await?;
    let frame = read_frame(&mut stream).await?;
    if frame.len() != 76 || i32_at(&frame, 4) != Some(11) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "CharacterCommand legado invalido",
        ));
    }
    let account = fixed_string(&frame[20..41])?;
    let character = fixed_string(&frame[41..73])?;
    if !tickets.validate_account(&account, &address) {
        stream.write_all(&selection_result(false)).await?;
        return Err(io::Error::new(
            io::ErrorKind::PermissionDenied,
            "ticket legado invalido",
        ));
    }
    let pool = pool.ok_or_else(|| io::Error::new(io::ErrorKind::NotConnected, "SQL ausente"))?;
    let repositories = SqlRepositories::new(pool);
    let service = LoginService::new(repositories.clone());
    if !service
        .character_is_owned(&account, &character)
        .await
        .map_err(io::Error::other)?
    {
        stream.write_all(&selection_result(false)).await?;
        return Ok(());
    }
    let selected = service
        .characters(&account)
        .await
        .map_err(io::Error::other)?
        .into_iter()
        .find(|value| value.name == character)
        .ok_or_else(|| io::Error::new(io::ErrorKind::NotFound, "personagem nao encontrado"))?;
    stream
        .write_all(&encode_character_record(&selected))
        .await?;
    let map_service = MapService::new(repositories);
    let npcs = map_service
        .npcs(selected.map)
        .await
        .map_err(io::Error::other)?;
    for (index, npc) in npcs.iter().enumerate() {
        let serial = 0x81000000u32 | ((npc.id.0 as u32) & 0x00ff_ffff);
        if let Some(packet) = encode_play_data_group(npc, serial, 0x80000000u32 | index as u32) {
            stream.write_all(&packet).await?;
        }
    }
    let mut incoming = sessions.subscribe();
    loop {
        let frame = tokio::select! {
            result = read_session_frame(&mut stream) => match result? {
                Some(frame) => frame,
                None => return Ok(()),
            },
            message = incoming.recv() => match message {
                Ok(packet) => { stream.write_all(&packet).await?; continue; }
                Err(broadcast::error::RecvError::Lagged(_)) => continue,
                Err(broadcast::error::RecvError::Closed) => return Ok(()),
            }
        };
        match i32_at(&frame, 4) {
            Some(63) if frame.len() == 24 => {
                let serial = u32_at(&frame, 12).unwrap_or_default();
                if let Some(npc) = npcs
                    .iter()
                    .find(|npc| (0x81000000u32 | ((npc.id.0 as u32) & 0x00ff_ffff)) == serial)
                {
                    if let Some(packet) = super::legacy::encode_npc_info(npc, serial) {
                        stream.write_all(&packet).await?;
                    }
                }
            }
            Some(22) if (9..=272).contains(&frame.len()) => {
                let text = frame[8..]
                    .split(|byte| *byte == 0)
                    .next()
                    .unwrap_or_default();
                if !text.is_empty() {
                    let message = String::from_utf8_lossy(text);
                    tracing::debug!(account = %account, message = %message, "chat legado recebido");
                    if is_notice_box_command(&message) {
                        if let Some(item_code) = notice_box_command(&account, &message) {
                            let _ = sessions.send(
                                super::legacy::encode_notice_box(item_code)
                                    .expect("item code was validated"),
                            );
                        }
                    } else {
                        let _ = sessions.send(super::legacy::encode_chat(&message, 0));
                    }
                }
            }
            Some(23) if frame.len() == 28 => {
                let position = crate::domain::MapPosition::new(
                    i32_at(&frame, 12).unwrap_or_default() as f32,
                    i32_at(&frame, 16).unwrap_or_default() as f32,
                    i32_at(&frame, 20).unwrap_or_default() as f32,
                );
                if position.is_finite() {
                    map_service
                        .save_position(&character, selected.map, position)
                        .await
                        .map_err(io::Error::other)?;
                }
            }
            Some(0) => {
                return Err(io::Error::new(
                    io::ErrorKind::InvalidData,
                    "opcode legado zero",
                ))
            }
            _ => {
                tracing::debug!(code = ?i32_at(&frame, 4), size = frame.len(), "frame legado ignorado no GameServer")
            }
        }
    }
}

fn is_notice_box_command(message: &str) -> bool {
    matches!(
        message.split_whitespace().next(),
        Some("/noticebox" | "/notice")
    )
}

fn notice_box_command(account: &str, message: &str) -> Option<u32> {
    let mut parts = message.split_whitespace();
    if !matches!(parts.next(), Some("/noticebox" | "/notice")) {
        return None;
    }
    let allowed = std::env::var("RUSTPT_GM_ACCOUNTS")
        .ok()
        .map(|value| value.split(',').any(|entry| entry.trim() == account))
        .unwrap_or(false);
    if !allowed {
        tracing::warn!(
            account,
            "NOTICE_BOX rejeitado: conta nao esta em RUSTPT_GM_ACCOUNTS"
        );
        return None;
    }
    parts
        .next()?
        .parse::<u32>()
        .ok()
        .filter(|code| *code > 0 && *code <= i32::MAX as u32)
}

async fn read_frame(stream: &mut TcpStream) -> io::Result<Vec<u8>> {
    let mut size = [0; 4];
    stream.read_exact(&mut size).await?;
    let size = i32::from_le_bytes(size);
    if size != 76 {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "tamanho de selecao invalido",
        ));
    }
    let mut frame = vec![0; size as usize];
    frame[..4].copy_from_slice(&size.to_le_bytes());
    stream.read_exact(&mut frame[4..]).await?;
    Ok(frame)
}

fn selection_result(success: bool) -> Vec<u8> {
    let mut frame = command(11);
    frame[12..16].copy_from_slice(&(success as i32).to_le_bytes());
    frame
}

fn command(code: i32) -> Vec<u8> {
    let mut bytes = vec![0; 24];
    bytes[0..4].copy_from_slice(&24i32.to_le_bytes());
    bytes[4..8].copy_from_slice(&code.to_le_bytes());
    bytes
}
fn i32_at(bytes: &[u8], offset: usize) -> Option<i32> {
    Some(i32::from_le_bytes(
        bytes.get(offset..offset + 4)?.try_into().ok()?,
    ))
}

fn u32_at(bytes: &[u8], offset: usize) -> Option<u32> {
    Some(u32::from_le_bytes(
        bytes.get(offset..offset + 4)?.try_into().ok()?,
    ))
}

async fn read_session_frame(stream: &mut TcpStream) -> io::Result<Option<Vec<u8>>> {
    let mut size = [0; 4];
    match stream.read_exact(&mut size).await {
        Ok(_) => {}
        Err(error) if error.kind() == io::ErrorKind::UnexpectedEof => return Ok(None),
        Err(error) => return Err(error),
    }
    let size = i32::from_le_bytes(size);
    if !(8..=4096).contains(&size) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "tamanho de frame legado invalido",
        ));
    }
    let mut frame = vec![0; size as usize];
    frame[..4].copy_from_slice(&size.to_le_bytes());
    stream.read_exact(&mut frame[4..]).await?;
    Ok(Some(frame))
}
fn fixed_string(bytes: &[u8]) -> io::Result<String> {
    let end = bytes.iter().position(|b| *b == 0).ok_or_else(|| {
        io::Error::new(io::ErrorKind::InvalidData, "string legado sem terminador")
    })?;
    String::from_utf8(bytes[..end].to_vec())
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "string legado invalida"))
}
