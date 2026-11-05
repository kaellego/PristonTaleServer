use crate::domain::MapPosition;
use crate::domain::NpcSnapshot;

pub const LEGACY_POSITION_CODE: i32 = 23;
pub const LEGACY_CHAT_CODE: i32 = 22;
pub const LEGACY_NOTICE_BOX_CODE: i32 = 201;

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct LegacyPosition {
    pub area: i32,
    pub position: MapPosition,
    pub hp: [i16; 2],
}

pub fn decode_position(payload: &[u8]) -> Option<LegacyPosition> {
    if payload.len() != 28 || read_i32(payload, 4)? != LEGACY_POSITION_CODE {
        return None;
    }
    let position = MapPosition::new(
        read_i32(payload, 12)? as f32,
        read_i32(payload, 16)? as f32,
        read_i32(payload, 20)? as f32,
    );
    position.is_finite().then_some(LegacyPosition {
        area: read_i32(payload, 8)?,
        position,
        hp: [read_i16(payload, 24)?, read_i16(payload, 26)?],
    })
}

pub fn encode_position(position: LegacyPosition) -> Vec<u8> {
    let mut bytes = vec![0; 28];
    put_i32(&mut bytes, 0, 28);
    put_i32(&mut bytes, 4, LEGACY_POSITION_CODE);
    put_i32(&mut bytes, 8, position.area);
    put_i32(&mut bytes, 12, position.position.x as i32);
    put_i32(&mut bytes, 16, position.position.y as i32);
    put_i32(&mut bytes, 20, position.position.z as i32);
    bytes[24..26].copy_from_slice(&position.hp[0].to_le_bytes());
    bytes[26..28].copy_from_slice(&position.hp[1].to_le_bytes());
    bytes
}

/// Server-to-client TRANS_CHATMESSAGE (the confirmed 272-byte legacy form).
pub fn encode_chat(message: &str, object_serial: u32) -> Vec<u8> {
    let mut packet = vec![0u8; 272];
    put_i32(&mut packet, 0, 272);
    put_i32(&mut packet, 4, LEGACY_CHAT_CODE);
    put_u32(&mut packet, 12, object_serial);
    put_fixed(&mut packet[16..272], message);
    packet
}

pub fn encode_notice_box(item_code: u32) -> Option<Vec<u8>> {
    if item_code == 0 || item_code > i32::MAX as u32 {
        return None;
    }
    let mut packet = vec![0u8; 24];
    put_i32(&mut packet, 0, 24);
    put_i32(&mut packet, 4, LEGACY_NOTICE_BOX_CODE);
    put_i32(&mut packet, 12, item_code as i32);
    Some(packet)
}

/// Converts a validated domain NPC to a future legacy packet representation.
/// No client-owned pointer or native C/C++ layout is accepted here.
pub fn encode_npc_info(npc: &NpcSnapshot, serial: u32) -> Option<Vec<u8>> {
    if !npc.is_valid() {
        return None;
    }
    let mut packet = vec![0u8; 504];
    put_i32(&mut packet, 0, 504);
    put_i32(&mut packet, 4, 63); // smTRANS_PLAYERINFO
    put_fixed(&mut packet[8..40], &npc.name);
    put_fixed(&mut packet[40..104], &npc.model);
    put_u32(&mut packet, 168, serial); // LegacyCharacterInfo.objectSerial + header
    put_i32(&mut packet, 176, 0); // NPC state
    put_i32(&mut packet, 180, 1); // normal size pattern
    put_u32(&mut packet, 188, 0xA0); // brood
    put_u32(&mut packet, 472, serial);
    put_i32(&mut packet, 476, npc.position.x as i32);
    put_i32(&mut packet, 480, npc.position.y as i32);
    put_i32(&mut packet, 484, npc.position.z as i32);
    Some(packet)
}

/// Legacy clients create world entities only after receiving PLAYDATAGROUP.
pub fn encode_play_data_group(npc: &NpcSnapshot, serial: u32, send_code: u32) -> Option<Vec<u8>> {
    if !npc.is_valid() {
        return None;
    }
    let mut packet = vec![0u8; 56];
    put_i32(&mut packet, 0, 56);
    put_i32(&mut packet, 4, 32);
    put_i32(&mut packet, 8, 1);
    put_u32(&mut packet, 12, send_code);
    put_i32(&mut packet, 16, 40);
    put_i32(&mut packet, 20, 28);
    put_u32(&mut packet, 24, serial);
    put_i32(&mut packet, 28, npc.position.x as i32);
    put_i32(&mut packet, 32, npc.position.y as i32);
    put_i32(&mut packet, 36, npc.position.z as i32);
    put_u32(&mut packet, 52, serial);
    Some(packet)
}

/// Encodes the confirmed 1768-byte legacy record envelope used on map entry.
pub fn encode_character_record(
    character: &crate::infrastructure::repositories::CharacterRecord,
) -> Vec<u8> {
    let mut packet = vec![0u8; 1800];
    put_i32(&mut packet, 0, 1800);
    put_i32(&mut packet, 4, 7);
    put_i32(&mut packet, 12, 1);
    put_i32(&mut packet, 16, 1768);
    put_i32(&mut packet, 20, 1768);
    put_i32(&mut packet, 24, 7);
    put_fixed(&mut packet[28..36], "RC 1.50");
    let base = 36;
    put_fixed(&mut packet[base..base + 32], &character.name);
    put_i32(&mut packet, base + 184, character.level.max(1));
    put_i32(&mut packet, base + 164, character.class_id);
    put_i32(&mut packet, base + 168, 0x80);
    put_u32(&mut packet, base + 332, 0xA0);
    let save = base + 472;
    put_i32(&mut packet, save + 4, character.map.0);
    put_i32(&mut packet, save + 12, character.position.x as i32);
    put_i32(&mut packet, save + 16, character.position.z as i32);
    packet
}

fn put_i32(bytes: &mut [u8], offset: usize, value: i32) {
    bytes[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
}
fn put_u32(bytes: &mut [u8], offset: usize, value: u32) {
    bytes[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
}
fn put_fixed(dst: &mut [u8], value: &str) {
    let bytes = value.as_bytes();
    let len = bytes.len().min(dst.len().saturating_sub(1));
    dst[..len].copy_from_slice(&bytes[..len]);
}
fn read_i32(bytes: &[u8], offset: usize) -> Option<i32> {
    Some(i32::from_le_bytes(
        bytes.get(offset..offset + 4)?.try_into().ok()?,
    ))
}
fn read_i16(bytes: &[u8], offset: usize) -> Option<i16> {
    Some(i16::from_le_bytes(
        bytes.get(offset..offset + 2)?.try_into().ok()?,
    ))
}

#[cfg(test)]
mod tests {
    use super::{
        decode_position, encode_character_record, encode_chat, encode_notice_box, encode_npc_info,
        encode_play_data_group, encode_position, LegacyPosition, LEGACY_CHAT_CODE,
        LEGACY_NOTICE_BOX_CODE,
    };
    use crate::domain::{MapId, MapPosition, NpcId, NpcSnapshot};

    #[test]
    fn position_codec_uses_confirmed_legacy_28_byte_layout() {
        let original = LegacyPosition {
            area: 3,
            position: MapPosition::new(-10.0, 20.0, 30.0),
            hp: [100, 90],
        };
        let decoded = decode_position(&encode_position(original)).unwrap();
        assert_eq!(decoded, original);
        assert!(decode_position(&[0; 27]).is_none());
    }
    #[test]
    fn npc_packet_matches_confirmed_legacy_size_and_offsets() {
        let npc = NpcSnapshot {
            id: NpcId(1),
            map: MapId(0),
            position: MapPosition::new(-10.0, 20.0, 30.0),
            name: "npc".into(),
            model: "npc.inx".into(),
            enabled: true,
        };
        let packet = encode_npc_info(&npc, 0x81000001).unwrap();
        assert_eq!(packet.len(), 504);
        assert_eq!(i32::from_le_bytes(packet[0..4].try_into().unwrap()), 504);
        assert_eq!(
            u32::from_le_bytes(packet[472..476].try_into().unwrap()),
            0x81000001
        );
        assert_eq!(
            i32::from_le_bytes(packet[476..480].try_into().unwrap()),
            -10
        );
    }

    #[test]
    fn map_entry_packets_have_confirmed_legacy_sizes() {
        let npc = NpcSnapshot {
            id: NpcId(1),
            map: MapId(0),
            position: MapPosition::new(1.0, 2.0, 3.0),
            name: "npc".into(),
            model: "npc.inx".into(),
            enabled: true,
        };
        assert_eq!(
            encode_play_data_group(&npc, 0x81000001, 0x80000001)
                .unwrap()
                .len(),
            56
        );
        let character = crate::infrastructure::repositories::CharacterRecord {
            id: crate::domain::CharacterId(1),
            name: "test".into(),
            account_name: "a".into(),
            experience: 0,
            state_version: 0,
            class_id: 1,
            level: 1,
            map: MapId(0),
            position: MapPosition::new(1.0, 2.0, 3.0),
        };
        assert_eq!(encode_character_record(&character).len(), 1800);
    }

    #[test]
    fn chat_packet_uses_confirmed_extended_layout() {
        let packet = encode_chat("hello", 0x81000001);
        assert_eq!(packet.len(), 272);
        assert_eq!(
            i32::from_le_bytes(packet[4..8].try_into().unwrap()),
            LEGACY_CHAT_CODE
        );
        assert_eq!(
            u32::from_le_bytes(packet[12..16].try_into().unwrap()),
            0x81000001
        );
        assert_eq!(&packet[16..21], b"hello");
    }

    #[test]
    fn notice_box_uses_confirmed_command_layout() {
        let packet = encode_notice_box(1234).unwrap();
        assert_eq!(packet.len(), 24);
        assert_eq!(
            i32::from_le_bytes(packet[4..8].try_into().unwrap()),
            LEGACY_NOTICE_BOX_CODE
        );
        assert_eq!(i32::from_le_bytes(packet[12..16].try_into().unwrap()), 1234);
        assert!(encode_notice_box(0).is_none());
    }
}
