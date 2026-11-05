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
    tracing_subscriber::fmt()
        .with_env_filter("rustpt_server=info")
        .init();
    let config = ServerConfig::from_ini_file("../server.ini").unwrap_or_default();
    let sql = if std::env::var("RUSTPT_SQL_HOST").is_ok() {
        let sql = SqlServerConfig::from_env().map_err(std::io::Error::other)?;
        let pool_size = std::env::var("RUSTPT_SQL_POOL_SIZE")
            .ok()
            .and_then(|value| value.parse::<usize>().ok())
            .filter(|size| (1..=256).contains(size))
            .unwrap_or_else(|| config.max_users.min(32) as usize);
        let pool = sql.pool(pool_size).map_err(std::io::Error::other)?;
        let mut client = pool.get().await.map_err(std::io::Error::other)?;
        client
            .simple_query("SELECT 1")
            .await
            .map_err(std::io::Error::other)?;
        tracing::info!("SQL Server connection validated");
        Some(pool)
    } else {
        None
    };
    let server = TcpServer::bind(("127.0.0.1", config.login_port), sql, config.max_users).await?;
    tracing::info!(address = %server.local_addr()?, "RustPT server listening");
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
