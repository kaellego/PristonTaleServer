use crate::domain::MapPosition;
use crate::domain::NpcSnapshot;

pub const LEGACY_POSITION_CODE: i32 = 23;

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
    use super::{decode_position, encode_npc_info, encode_position, LegacyPosition};
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
}
