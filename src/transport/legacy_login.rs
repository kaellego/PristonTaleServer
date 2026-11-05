use crate::infrastructure::repositories::CharacterRecord;

pub const OP_GET_USER_INFO: i32 = 8;
pub const OP_FAIL_CONNECT: i32 = 64;
pub const OP_SET_USER_INFO: i32 = 9;
pub const OP_SERVER_INFO: i32 = 96;
pub const LOGIN_PACKET_SIZE: usize = 124;
pub const LEGACY_SERVER_LIST_SIZE: usize = 908;
pub const LEGACY_USER_INFO_SIZE: usize = 1_473;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct LegacyLoginPacket {
    pub account: String,
    pub password: String,
    pub hash: String,
    pub mac: String,
}

pub fn decode_login_packet(bytes: &[u8]) -> Option<LegacyLoginPacket> {
    if bytes.len() != LOGIN_PACKET_SIZE
        || read_i32(bytes, 0)? != LOGIN_PACKET_SIZE as i32
        || read_i32(bytes, 4)? != OP_GET_USER_INFO
    {
        return None;
    }
    Some(LegacyLoginPacket {
        account: fixed_string(&bytes[8..29])?,
        password: fixed_string(&bytes[29..61])?,
        hash: fixed_string(&bytes[61..94])?,
        mac: fixed_string(&bytes[94..124])?,
    })
}

pub fn encode_fail_connect(code: i32) -> Vec<u8> {
    let mut bytes = vec![0; 24];
    put_i32(&mut bytes, 0, 24);
    put_i32(&mut bytes, 4, OP_FAIL_CONNECT);
    put_i32(&mut bytes, 12, code);
    put_i32(&mut bytes, 20, 1);
    bytes
}

pub fn encode_server_info(server_name: &str, name: &str, ip: &str, port: u32) -> Vec<u8> {
    let mut bytes = vec![0; LEGACY_SERVER_LIST_SIZE];
    put_i32(&mut bytes, 0, LEGACY_SERVER_LIST_SIZE as i32);
    put_i32(&mut bytes, 4, OP_SERVER_INFO);
    put_fixed(&mut bytes[8..24], server_name);
    put_i32(&mut bytes, 40, 1);
    let server = 48;
    put_fixed(&mut bytes[server..server + 32], name);
    put_fixed(&mut bytes[server + 32..server + 52], ip);
    put_fixed(&mut bytes[server + 52..server + 72], ip);
    put_fixed(&mut bytes[server + 72..server + 92], ip);
    put_u32(&mut bytes, server + 92, port);
    put_u32(&mut bytes, server + 96, port);
    put_u32(&mut bytes, server + 100, port);
    bytes
}

pub fn encode_user_info(account: &str, characters: &[CharacterRecord]) -> Vec<u8> {
    let mut bytes = vec![0; LEGACY_USER_INFO_SIZE];
    put_i32(&mut bytes, 0, LEGACY_USER_INFO_SIZE as i32);
    put_i32(&mut bytes, 4, OP_SET_USER_INFO);
    put_fixed(&mut bytes[8..29], account);
    let count = characters.len().min(6);
    put_i32(&mut bytes, 29, count as i32);
    const CHARACTER_SIZE: usize = 240;
    for (index, character) in characters.iter().take(count).enumerate() {
        let offset = 33 + index * CHARACTER_SIZE;
        put_fixed(&mut bytes[offset..offset + 32], &character.name);
        put_i32(&mut bytes, offset + 32 + 64 + 64, 1);
        put_i32(&mut bytes, offset + 32 + 64 + 64 + 4, 1);
        put_i32(&mut bytes, offset + 32 + 64 + 64 + 8, 0);
    }
    bytes
}

fn fixed_string(bytes: &[u8]) -> Option<String> {
    let end = bytes.iter().position(|byte| *byte == 0)?;
    std::str::from_utf8(&bytes[..end]).ok().map(str::to_owned)
}

fn put_fixed(dst: &mut [u8], value: &str) {
    let length = value.len().min(dst.len().saturating_sub(1));
    dst[..length].copy_from_slice(&value.as_bytes()[..length]);
}

fn read_i32(bytes: &[u8], offset: usize) -> Option<i32> {
    Some(i32::from_le_bytes(
        bytes.get(offset..offset + 4)?.try_into().ok()?,
    ))
}

fn put_i32(bytes: &mut [u8], offset: usize, value: i32) {
    bytes[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
}

fn put_u32(bytes: &mut [u8], offset: usize, value: u32) {
    bytes[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::domain::CharacterId;

    #[test]
    fn decodes_confirmed_legacy_login_layout() {
        let mut bytes = vec![0; LOGIN_PACKET_SIZE];
        put_i32(&mut bytes, 0, LOGIN_PACKET_SIZE as i32);
        put_i32(&mut bytes, 4, OP_GET_USER_INFO);
        put_fixed(&mut bytes[8..29], "test");
        put_fixed(&mut bytes[29..61], "password");
        put_fixed(&mut bytes[61..94], "hash");
        put_fixed(&mut bytes[94..124], "mac");
        assert_eq!(decode_login_packet(&bytes).unwrap().account, "test");
        assert!(decode_login_packet(&bytes[..123]).is_none());
    }

    #[test]
    fn encodes_legacy_response_sizes() {
        let characters = vec![CharacterRecord {
            id: CharacterId(1),
            name: "mech".into(),
            account_name: "test".into(),
            experience: 0,
            state_version: 1,
        }];
        assert_eq!(encode_fail_connect(-2).len(), 24);
        assert_eq!(
            encode_server_info("Beta", "Awell", "127.0.0.1", 5056).len(),
            LEGACY_SERVER_LIST_SIZE
        );
        assert_eq!(
            encode_user_info("test", &characters).len(),
            LEGACY_USER_INFO_SIZE
        );
    }
}
