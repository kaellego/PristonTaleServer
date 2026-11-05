use std::io;

use deadpool_tiberius::Pool;
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpListener, TcpStream},
};

use super::ticket::TicketStore;
use crate::{application::services::LoginService, infrastructure::repositories::SqlRepositories};

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
        loop {
            let (stream, peer) = self.listener.accept().await?;
            let pool = self.pool.clone();
            let tickets = self.tickets.clone();
            tokio::spawn(async move {
                if let Err(error) = handle(stream, peer.ip().to_string(), pool, tickets).await {
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
    let service = LoginService::new(SqlRepositories::new(pool));
    if !service
        .character_is_owned(&account, &character)
        .await
        .map_err(io::Error::other)?
    {
        stream.write_all(&selection_result(false)).await?;
        return Ok(());
    }
    stream.write_all(&selection_result(true)).await?;
    Ok(())
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
fn fixed_string(bytes: &[u8]) -> io::Result<String> {
    let end = bytes.iter().position(|b| *b == 0).ok_or_else(|| {
        io::Error::new(io::ErrorKind::InvalidData, "string legado sem terminador")
    })?;
    String::from_utf8(bytes[..end].to_vec())
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "string legado invalida"))
}
