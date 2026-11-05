use std::{fs, io, path::Path};

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ServerConfig {
    pub login_port: u16,
    pub game_port: u16,
    pub max_users: u32,
}

impl Default for ServerConfig {
    fn default() -> Self {
        Self {
            login_port: 5055,
            game_port: 5056,
            max_users: 550,
        }
    }
}

impl ServerConfig {
    pub fn from_ini_file(path: impl AsRef<Path>) -> io::Result<Self> {
        let contents = fs::read_to_string(path)?;
        Self::from_ini(&contents)
            .map_err(|message| io::Error::new(io::ErrorKind::InvalidData, message))
    }

    pub fn from_ini(contents: &str) -> Result<Self, String> {
        let mut config = Self::default();
        let mut section = String::new();
        for (line_number, raw) in contents.lines().enumerate() {
            let line = raw.split(';').next().unwrap_or_default().trim();
            if line.is_empty() {
                continue;
            }
            if line.starts_with('[') && line.ends_with(']') {
                section = line[1..line.len() - 1].trim().to_ascii_lowercase();
                continue;
            }
            let Some((key, value)) = line.split_once('=') else {
                return Err(format!("linha {} sem '='", line_number + 1));
            };
            let key = key.trim().to_ascii_lowercase();
            let value = value.trim();
            match (section.as_str(), key.as_str()) {
                ("loginserver", "port") => config.login_port = parse_port(value, line_number)?,
                ("gameserver1", "port") => config.game_port = parse_port(value, line_number)?,
                ("server", "maxusers") => {
                    config.max_users = value
                        .parse()
                        .map_err(|_| format!("MaxUsers inválido na linha {}", line_number + 1))?
                }
                _ => {}
            }
        }
        if config.login_port == config.game_port {
            return Err("LoginServer e GameServer não podem usar a mesma porta".into());
        }
        if config.max_users == 0 {
            return Err("MaxUsers deve ser maior que zero".into());
        }
        Ok(config)
    }
}

fn parse_port(value: &str, line: usize) -> Result<u16, String> {
    let port: u16 = value
        .parse()
        .map_err(|_| format!("porta inválida na linha {}", line + 1))?;
    if port == 0 {
        return Err(format!("porta inválida na linha {}", line + 1));
    }
    Ok(port)
}

#[cfg(test)]
mod tests {
    use super::ServerConfig;

    #[test]
    fn reads_ports_and_user_limit() {
        let config = ServerConfig::from_ini(
            "[Server]\nMaxUsers = 100\n[LoginServer]\nPort = 6000\n[GameServer1]\nPort = 6001",
        )
        .unwrap();
        assert_eq!(
            config,
            ServerConfig {
                login_port: 6000,
                game_port: 6001,
                max_users: 100
            }
        );
    }

    #[test]
    fn rejects_duplicate_ports() {
        assert!(
            ServerConfig::from_ini("[LoginServer]\nPort=5055\n[GameServer1]\nPort=5055").is_err()
        );
    }
}
