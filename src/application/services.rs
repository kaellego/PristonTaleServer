use crate::{
    domain::{MapId, MapPosition, NpcId, NpcSnapshot},
    infrastructure::repositories::{
        AccountRecord, AccountRepository, CharacterRecord, CharacterRepository, NpcRepository,
        ShopRepository,
    },
};

pub struct LoginService<R> {
    repositories: R,
}
pub struct MapService<R> {
    repositories: R,
}
pub struct ShopService<R> {
    repositories: R,
}

impl<R> LoginService<R> {
    pub fn new(repositories: R) -> Self {
        Self { repositories }
    }
}
impl<R> MapService<R> {
    pub fn new(repositories: R) -> Self {
        Self { repositories }
    }
}
impl<R> ShopService<R> {
    pub fn new(repositories: R) -> Self {
        Self { repositories }
    }
}

impl<R: AccountRepository + CharacterRepository> LoginService<R> {
    pub async fn account(
        &self,
        name: &str,
    ) -> Result<Option<AccountRecord>, tiberius::error::Error> {
        self.repositories.find_account(name).await
    }
    pub async fn characters(
        &self,
        account: &str,
    ) -> Result<Vec<CharacterRecord>, tiberius::error::Error> {
        self.repositories.list_characters(account).await
    }
    pub async fn character_is_owned(
        &self,
        account: &str,
        character: &str,
    ) -> Result<bool, tiberius::error::Error> {
        self.repositories
            .belongs_to_account(account, character)
            .await
    }
}

impl<R: NpcRepository> MapService<R> {
    pub async fn npcs(&self, map: MapId) -> Result<Vec<NpcSnapshot>, tiberius::error::Error> {
        self.repositories.list_for_map(map).await
    }

    pub async fn save_position(
        &self,
        character: &str,
        map: MapId,
        position: MapPosition,
    ) -> Result<(), Box<dyn std::error::Error + Send + Sync>>
    where
        R: CharacterRepository,
    {
        self.repositories
            .save_position(character, map, position)
            .await
    }
}

impl<R: ShopRepository> ShopService<R> {
    pub async fn buy(
        &self,
        character: &str,
        npc: NpcId,
        item: i32,
        quantity: i32,
    ) -> Result<(), Box<dyn std::error::Error + Send + Sync>> {
        self.repositories.buy(character, npc, item, quantity).await
    }
}
