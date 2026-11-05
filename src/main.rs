mod application;
mod auth;
mod config;
mod domain;
mod infrastructure;
mod transport;

use config::ServerConfig;
use infrastructure::sql::SqlServerConfig;
use transport::tcp::TcpServer;

#[tokio::main]
async fn main() -> std::io::Result<()> {
    let config = ServerConfig::from_ini_file("../server.ini").unwrap_or_default();
    let sql = if std::env::var("ZEROPT_SQL_HOST").is_ok() {
        let sql = SqlServerConfig::from_env().map_err(std::io::Error::other)?;
        let mut client = sql.connect().await.map_err(std::io::Error::other)?;
        client
            .simple_query("SELECT 1")
            .await
            .map_err(std::io::Error::other)?;
        println!("SQL Server connection validated");
        Some(sql)
    } else {
        None
    };
    let server = TcpServer::bind(("127.0.0.1", config.login_port), sql).await?;
    println!(
        "RustPT Rust TCP server listening on {}",
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
