use crate::domain::NpcSnapshot;

/// Converts a validated domain NPC to a future legacy packet representation.
/// No client-owned pointer or native C/C++ layout is accepted here.
pub fn encode_npc_placeholder(npc: &NpcSnapshot) -> Option<Vec<u8>> {
    if !npc.is_valid() {
        return None;
    }

    // Packet layout is intentionally not guessed before it is captured and documented.
    Some(Vec::new())
}
