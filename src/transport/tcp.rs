use std::{io, net::SocketAddr};

use crate::auth::{verify, NONCE_SIZE};
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpListener, TcpStream},
};

const HEADER_SIZE: usize = 8;
const MAX_FRAME_SIZE: usize = 1024 * 1024;
const OPCODE_AUTHENTICATE: u32 = 2;
const OPCODE_AUTH_OK: u32 = 3;

#[derive(Debug, PartialEq, Eq)]
pub struct Frame {
    pub opcode: u32,
    pub payload: Vec<u8>,
}

pub struct TcpServer {
    listener: TcpListener,
}

impl TcpServer {
    pub async fn bind(address: impl tokio::net::ToSocketAddrs) -> io::Result<Self> {
        Ok(Self {
            listener: TcpListener::bind(address).await?,
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
            tokio::spawn(async move {
                if let Err(error) = handle_connection(stream, secret.as_bytes()).await {
                    eprintln!("TCP connection {peer} closed: {error}");
                }
            });
        }
    }
}

async fn handle_connection(mut stream: TcpStream, secret: &[u8]) -> io::Result<()> {
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
