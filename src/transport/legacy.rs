use crate::domain::NpcSnapshot;

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

#[cfg(test)]
mod tests {
    use super::encode_npc_info;
    use crate::domain::{MapId, MapPosition, NpcId, NpcSnapshot};
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
