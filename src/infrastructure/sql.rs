use std::env;

use tiberius::{AuthMethod, Client, Config, EncryptionLevel};
use tokio::net::TcpStream;
use tokio_util::compat::{Compat, TokioAsyncWriteCompatExt};

pub type SqlClient = Client<Compat<TcpStream>>;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct SqlServerConfig {
    pub host: String,
    pub port: u16,
    pub database: String,
    pub user: String,
    pub password: String,
    pub trust_server_certificate: bool,
}

impl SqlServerConfig {
    pub fn from_env() -> Result<Self, String> {
        Ok(Self {
            host: required("ZEROPT_SQL_HOST")?,
            port: env::var("ZEROPT_SQL_PORT")
                .unwrap_or_else(|_| "1433".into())
                .parse()
                .map_err(|_| "ZEROPT_SQL_PORT inválida".to_string())?,
            database: required("ZEROPT_SQL_DATABASE")?,
            user: required("ZEROPT_SQL_USER")?,
            password: required("ZEROPT_SQL_PASSWORD")?,
            trust_server_certificate: env::var("ZEROPT_SQL_TRUST_CERT").as_deref() == Ok("1"),
        })
    }

    pub async fn connect(&self) -> Result<SqlClient, Box<dyn std::error::Error + Send + Sync>> {
        let mut config = Config::new();
        config.host(&self.host);
        config.port(self.port);
        config.database(&self.database);
        config.authentication(AuthMethod::sql_server(&self.user, &self.password));
        config.encryption(if self.trust_server_certificate {
            EncryptionLevel::Required
        } else {
            EncryptionLevel::NotSupported
        });
        if self.trust_server_certificate {
            config.trust_cert();
        }

        let tcp = TcpStream::connect((self.host.as_str(), self.port)).await?;
        tcp.set_nodelay(true)?;
        Ok(Client::connect(config, tcp.compat_write()).await?)
    }
}

fn required(name: &str) -> Result<String, String> {
    env::var(name).map_err(|_| format!("variável {name} ausente"))
}

#[cfg(test)]
mod tests {
    use super::SqlServerConfig;

    #[test]
    fn config_requires_external_credentials() {
        std::env::remove_var("ZEROPT_SQL_HOST");
        assert!(SqlServerConfig::from_env().is_err());
    }
}
