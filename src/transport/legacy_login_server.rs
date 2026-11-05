use std::io;

use deadpool_tiberius::Pool;
use sha2::{Digest, Sha256};
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpListener, TcpStream},
};

use super::legacy_login::{
    decode_login_packet, encode_fail_connect, encode_server_info, encode_user_info,
};
use crate::{application::services::LoginService, infrastructure::repositories::SqlRepositories};

pub struct LegacyLoginServer {
    listener: TcpListener,
    pool: Option<Pool>,
}

impl LegacyLoginServer {
    pub async fn bind(
        address: impl tokio::net::ToSocketAddrs,
        pool: Option<Pool>,
    ) -> io::Result<Self> {
        Ok(Self {
            listener: TcpListener::bind(address).await?,
            pool,
        })
    }

    pub async fn run(self) -> io::Result<()> {
        loop {
            let (stream, _) = self.listener.accept().await?;
            let pool = self.pool.clone();
            tokio::spawn(async move {
                if let Err(error) = handle(stream, pool).await {
                    tracing::debug!(%error, "legacy login connection closed");
                }
            });
        }
    }
}

async fn handle(mut stream: TcpStream, pool: Option<Pool>) -> io::Result<()> {
    stream.write_all(&command(13)).await?;
    let payload = read_legacy_frame(&mut stream).await?;
    let login = decode_login_packet(&payload)
        .ok_or_else(|| io::Error::new(io::ErrorKind::InvalidData, "login legado invalido"))?;
    let pool = pool.ok_or_else(|| io::Error::new(io::ErrorKind::NotConnected, "SQL ausente"))?;
    let repo = SqlRepositories::new(pool);
    let service = LoginService::new(repo.clone());
    let account = service
        .account(&login.account)
        .await
        .map_err(io::Error::other)?
        .ok_or_else(|| io::Error::new(io::ErrorKind::PermissionDenied, "conta inexistente"))?;
    if account.banned || !password_matches(&login.password, &account.password) {
        stream.write_all(&encode_fail_connect(-2)).await?;
        return Ok(());
    }
    let characters = service
        .characters(&login.account)
        .await
        .map_err(io::Error::other)?;
    stream
        .write_all(&encode_user_info(&login.account, &characters))
        .await?;
    stream
        .write_all(&encode_server_info("Beta", "Awell", "127.0.0.1", 5056))
        .await?;
    Ok(())
}

async fn read_legacy_frame(stream: &mut TcpStream) -> io::Result<Vec<u8>> {
    let mut size = [0; 4];
    stream.read_exact(&mut size).await?;
    let size = i32::from_le_bytes(size);
    if !(8..=4096).contains(&size) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "tamanho legado invalido",
        ));
    }
    let mut payload = vec![0; size as usize - 4];
    stream.read_exact(&mut payload).await?;
    let mut frame = size.to_le_bytes().to_vec();
    frame.extend_from_slice(&payload);
    Ok(frame)
}

fn command(code: i32) -> Vec<u8> {
    let mut bytes = vec![0; 24];
    bytes[0..4].copy_from_slice(&24i32.to_le_bytes());
    bytes[4..8].copy_from_slice(&code.to_le_bytes());
    bytes
}

fn password_matches(received: &str, stored: &str) -> bool {
    if stored.len() == 64 && stored.bytes().all(|byte| byte.is_ascii_hexdigit()) {
        let digest = Sha256::digest(received.as_bytes());
        let calculated = digest
            .iter()
            .map(|byte| format!("{byte:02x}"))
            .collect::<String>();
        calculated.eq_ignore_ascii_case(stored)
    } else {
        received == stored
    }
}
