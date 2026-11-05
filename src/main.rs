mod application;
mod auth;
mod config;
mod domain;
mod infrastructure;
mod transport;

use config::ServerConfig;
use transport::tcp::TcpServer;

#[tokio::main]
async fn main() -> std::io::Result<()> {
    let config = ServerConfig::from_ini_file("../server.ini").unwrap_or_default();
    let server = TcpServer::bind(("127.0.0.1", config.login_port)).await?;
    println!(
        "ZeroPT Rust TCP server listening on {}",
        server.local_addr()?
    );
    server.run().await
}

#[cfg(test)]
mod tests {
    use crate::domain::{MapId, MapPosition, NpcId, NpcSnapshot};

    #[test]
    fn npc_snapshot_rejects_invalid_position() {
        let npc = NpcSnapshot {
            id: NpcId(10000),
            map: MapId(0),
            position: MapPosition::new(f32::NAN, 0.0, 0.0),
            name: "test".into(),
            model: "test.inx".into(),
            enabled: true,
        };

        assert!(!npc.is_valid());
    }
}
