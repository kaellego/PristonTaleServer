use crate::{
    domain::{MapId, NpcId, NpcSnapshot},
    infrastructure::{repositories, sql::SqlClient},
};

pub struct LoginService;
pub struct MapService;
pub struct ShopService;

impl LoginService {
    pub async fn character_is_owned(
        client: &mut SqlClient,
        account: &str,
        character: &str,
    ) -> Result<bool, tiberius::error::Error> {
        repositories::character_belongs_to_account(client, account, character).await
    }
}

impl MapService {
    pub async fn npcs(
        client: &mut SqlClient,
        map: MapId,
    ) -> Result<Vec<NpcSnapshot>, tiberius::error::Error> {
        repositories::list_npcs_for_map(client, map).await
    }
}

impl ShopService {
    pub async fn buy(
        client: &mut SqlClient,
        character: &str,
        npc: NpcId,
        item: i32,
        quantity: i32,
    ) -> Result<(), Box<dyn std::error::Error + Send + Sync>> {
        repositories::buy_item(client, character, npc, item, quantity).await
    }
}
