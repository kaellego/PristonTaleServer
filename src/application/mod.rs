use crate::domain::{MapId, NpcSnapshot};
pub mod services;

pub struct MapEntryService;

impl MapEntryService {
    pub fn npcs_for_map<'a>(npcs: &'a [NpcSnapshot], map: MapId) -> Vec<&'a NpcSnapshot> {
        npcs.iter()
            .filter(|npc| npc.enabled && npc.map == map && npc.is_valid())
            .collect()
    }
}

#[cfg(test)]
mod tests {
    use super::MapEntryService;
    use crate::domain::{MapId, MapPosition, NpcId, NpcSnapshot};

    #[test]
    fn map_entry_returns_only_enabled_npcs_from_target_map() {
        let npcs = vec![
            NpcSnapshot {
                id: NpcId(1),
                map: MapId(2),
                position: MapPosition::new(1.0, 2.0, 3.0),
                name: "npc".into(),
                model: "npc.inx".into(),
                enabled: true,
            },
            NpcSnapshot {
                id: NpcId(2),
                map: MapId(3),
                position: MapPosition::new(1.0, 2.0, 3.0),
                name: "other".into(),
                model: "other.inx".into(),
                enabled: true,
            },
        ];

        assert_eq!(MapEntryService::npcs_for_map(&npcs, MapId(2)).len(), 1);
    }
}
