#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct AccountId(pub i32);

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct CharacterId(pub i32);

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct MapId(pub i32);

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct NpcId(pub i32);

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct MapPosition {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

impl MapPosition {
    pub const fn new(x: f32, y: f32, z: f32) -> Self {
        Self { x, y, z }
    }

    pub fn is_finite(self) -> bool {
        self.x.is_finite() && self.y.is_finite() && self.z.is_finite()
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct NpcSnapshot {
    pub id: NpcId,
    pub map: MapId,
    pub position: MapPosition,
    pub name: String,
    pub model: String,
    pub enabled: bool,
}

impl NpcSnapshot {
    pub fn is_valid(&self) -> bool {
        self.id.0 > 0
            && self.map.0 >= 0
            && self.position.is_finite()
            && !self.name.is_empty()
            && !self.model.is_empty()
            && self.name.len() < 32
            && self.model.len() < 128
    }
}
