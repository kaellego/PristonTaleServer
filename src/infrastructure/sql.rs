use std::env;

use deadpool_tiberius::{Manager, Pool};
use tiberius::EncryptionLevel;

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

    pub fn pool(&self, max_size: usize) -> Result<Pool, Box<dyn std::error::Error + Send + Sync>> {
        let mut manager = Manager::new()
            .host(&self.host)
            .port(self.port)
            .database(&self.database)
            .basic_authentication(&self.user, &self.password)
            .encryption(if self.trust_server_certificate {
                EncryptionLevel::Required
            } else {
                EncryptionLevel::NotSupported
            });
        if self.trust_server_certificate {
            manager = manager.trust_cert();
        }
        Ok(manager.max_size(max_size).create_pool()?)
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
