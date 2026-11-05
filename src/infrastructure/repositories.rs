use crate::domain::{AccountId, CharacterId, MapId, MapPosition, NpcId, NpcSnapshot};
use async_trait::async_trait;
use deadpool_tiberius::Client as SqlClient;
use deadpool_tiberius::Pool;
use tiberius::Row;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct AccountRecord {
    pub id: AccountId,
    pub name: String,
    pub banned: bool,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CharacterRecord {
    pub id: CharacterId,
    pub name: String,
    pub account_name: String,
    pub experience: i64,
    pub state_version: i32,
}

#[derive(Debug, PartialEq, Eq)]
pub enum ShopError {
    InvalidQuantity,
    NotForSale,
    InsufficientGold,
    InventoryFull,
}

fn boxed<E: std::error::Error + Send + Sync + 'static>(
    error: E,
) -> Box<dyn std::error::Error + Send + Sync> {
    Box::new(error)
}

impl std::fmt::Display for ShopError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{self:?}")
    }
}

impl std::error::Error for ShopError {}

pub async fn buy_item(
    client: &mut SqlClient,
    character_name: &str,
    npc_id: NpcId,
    item_code: i32,
    quantity: i32,
) -> Result<(), Box<dyn std::error::Error + Send + Sync>> {
    if quantity <= 0 {
        return Err(ShopError::InvalidQuantity.into());
    }
    client.simple_query("BEGIN TRANSACTION").await?;
    let result: Result<(), Box<dyn std::error::Error + Send + Sync>> = async {
        let price_rows = client.query("SELECT i.BuyPrice, i.MaxQuantity FROM GameDB.dbo.NpcShop s INNER JOIN GameDB.dbo.NpcShopItem i WITH (UPDLOCK, HOLDLOCK) ON i.ShopId = s.ShopId WHERE s.NpcId = @P1 AND s.Enabled = 1 AND i.ItemCode = @P2 AND i.Enabled = 1 AND (i.Stock IS NULL OR i.Stock >= @P3)", &[&npc_id.0, &item_code, &quantity]).await.map_err(boxed)?;
        let row = price_rows.into_first_result().await.map_err(boxed)?.into_iter().next().ok_or_else(|| boxed(ShopError::NotForSale))?;
        let unit_price: i64 = row.get(0).ok_or_else(|| boxed(ShopError::NotForSale))?;
        let max_quantity: i32 = row.get(1).ok_or_else(|| boxed(ShopError::NotForSale))?;
        if quantity > max_quantity { return Err(boxed(ShopError::InvalidQuantity)); }
        let total = unit_price.checked_mul(quantity as i64).ok_or_else(|| boxed(ShopError::InvalidQuantity))?;
        let debit = client.execute("UPDATE UserDB.dbo.CharacterInfo SET Gold = Gold - @P1 WHERE Name = @P2 AND Gold >= @P1", &[&total, &character_name]).await.map_err(boxed)?;
        if debit.rows_affected().iter().sum::<u64>() != 1 { return Err(boxed(ShopError::InsufficientGold)); }
        let insert = client.execute("INSERT INTO UserDB.dbo.CharacterInventory (CharacterName, SlotIndex, ItemCode, Quantity) SELECT TOP (1) @P1, v.SlotIndex, @P2, @P3 FROM (SELECT TOP (60) ROW_NUMBER() OVER (ORDER BY (SELECT NULL)) - 1 AS SlotIndex FROM sys.all_objects) v WHERE NOT EXISTS (SELECT 1 FROM UserDB.dbo.CharacterInventory c WHERE c.CharacterName = @P1 AND c.SlotIndex = v.SlotIndex)", &[&character_name, &item_code, &quantity]).await.map_err(boxed)?;
        if insert.rows_affected().iter().sum::<u64>() != 1 { return Err(boxed(ShopError::InventoryFull)); }
        client.execute("UPDATE i SET i.Stock = i.Stock - @P1 FROM GameDB.dbo.NpcShopItem i INNER JOIN GameDB.dbo.NpcShop s ON s.ShopId = i.ShopId WHERE s.NpcId = @P2 AND i.ItemCode = @P3 AND i.Stock IS NOT NULL", &[&quantity, &npc_id.0, &item_code]).await.map_err(boxed)?;
        Ok(())
    }.await;
    match result {
        Ok(()) => {
            client.simple_query("COMMIT TRANSACTION").await?;
            Ok(())
        }
        Err(error) => {
            let _ = client.simple_query("ROLLBACK TRANSACTION").await;
            Err(error)
        }
    }
}

pub async fn find_account(
    client: &mut SqlClient,
    name: &str,
) -> Result<Option<AccountRecord>, tiberius::error::Error> {
    let rows = client
        .query(
            "SELECT ID, AccountName, BanStatus FROM UserDB.dbo.UserInfo WHERE AccountName = @P1",
            &[&name],
        )
        .await?;
    Ok(rows
        .into_first_result()
        .await?
        .into_iter()
        .next()
        .map(account_row))
}

pub async fn list_characters(
    client: &mut SqlClient,
    account_name: &str,
) -> Result<Vec<CharacterRecord>, tiberius::error::Error> {
    let rows = client
        .query("SELECT Name, AccountName, Experience, StateVersion FROM UserDB.dbo.CharacterInfo WHERE AccountName = @P1 ORDER BY Experience DESC, Name", &[&account_name])
        .await?;
    Ok(rows
        .into_first_result()
        .await?
        .into_iter()
        .enumerate()
        .map(|(index, row)| character_row(index, row))
        .collect())
}

pub async fn character_belongs_to_account(
    client: &mut SqlClient,
    account_name: &str,
    character_name: &str,
) -> Result<bool, tiberius::error::Error> {
    let rows = client
        .query(
            "SELECT 1 FROM UserDB.dbo.CharacterInfo WHERE AccountName = @P1 AND Name = @P2",
            &[&account_name, &character_name],
        )
        .await?;
    Ok(!rows.into_first_result().await?.is_empty())
}

pub async fn list_npcs_for_map(
    client: &mut SqlClient,
    map: MapId,
) -> Result<Vec<NpcSnapshot>, tiberius::error::Error> {
    let rows = client
        .query("SELECT n.NpcId, s.MapId, s.X, s.Y, s.Z, n.DisplayName, n.ModelName, n.Enabled & s.Enabled AS Enabled FROM GameDB.dbo.NpcList n INNER JOIN GameDB.dbo.NpcSpawn s ON s.NpcId = n.NpcId WHERE s.MapId = @P1 AND n.Enabled = 1 AND s.Enabled = 1 AND s.X IS NOT NULL AND s.Y IS NOT NULL AND s.Z IS NOT NULL ORDER BY s.SpawnId", &[&map.0])
        .await?;
    Ok(rows
        .into_first_result()
        .await?
        .into_iter()
        .filter_map(npc_row)
        .filter(NpcSnapshot::is_valid)
        .collect())
}

fn account_row(row: Row) -> AccountRecord {
    AccountRecord {
        id: AccountId(row.get(0).unwrap_or_default()),
        name: row.get::<&str, _>(1).unwrap_or_default().to_owned(),
        banned: row.get::<i32, _>(2).unwrap_or_default() != 0,
    }
}

fn character_row(index: usize, row: Row) -> CharacterRecord {
    CharacterRecord {
        id: CharacterId(index as i32),
        name: row.get::<&str, _>(0).unwrap_or_default().to_owned(),
        account_name: row.get::<&str, _>(1).unwrap_or_default().to_owned(),
        experience: row.get(2).unwrap_or_default(),
        state_version: row.get(3).unwrap_or_default(),
    }
}

fn npc_row(row: Row) -> Option<NpcSnapshot> {
    Some(NpcSnapshot {
        id: NpcId(row.get(0)?),
        map: MapId(row.get(1)?),
        position: MapPosition::new(
            row.get::<i32, _>(2)? as f32,
            row.get::<i32, _>(3)? as f32,
            row.get::<i32, _>(4)? as f32,
        ),
        name: row.get::<&str, _>(5)?.to_owned(),
        model: row.get::<&str, _>(6)?.to_owned(),
        enabled: row.get::<bool, _>(7)?,
    })
}

#[async_trait]
pub trait AccountRepository: Send + Sync {
    async fn find_account(
        &self,
        name: &str,
    ) -> Result<Option<AccountRecord>, tiberius::error::Error>;
}

#[async_trait]
pub trait CharacterRepository: Send + Sync {
    async fn list_characters(
        &self,
        account: &str,
    ) -> Result<Vec<CharacterRecord>, tiberius::error::Error>;
    async fn belongs_to_account(
        &self,
        account: &str,
        character: &str,
    ) -> Result<bool, tiberius::error::Error>;
    async fn save_position(
        &self,
        character: &str,
        map: MapId,
        position: MapPosition,
    ) -> Result<(), Box<dyn std::error::Error + Send + Sync>>;
}

#[async_trait]
pub trait NpcRepository: Send + Sync {
    async fn list_for_map(&self, map: MapId) -> Result<Vec<NpcSnapshot>, tiberius::error::Error>;
}

#[async_trait]
pub trait ShopRepository: Send + Sync {
    async fn buy(
        &self,
        character: &str,
        npc: NpcId,
        item: i32,
        quantity: i32,
    ) -> Result<(), Box<dyn std::error::Error + Send + Sync>>;
}

#[derive(Clone)]
pub struct SqlRepositories {
    pool: Pool,
}

impl SqlRepositories {
    pub fn new(pool: Pool) -> Self {
        Self { pool }
    }
}

#[async_trait]
impl AccountRepository for SqlRepositories {
    async fn find_account(
        &self,
        name: &str,
    ) -> Result<Option<AccountRecord>, tiberius::error::Error> {
        let mut client = self
            .pool
            .get()
            .await
            .map_err(|error| tiberius::error::Error::from(std::io::Error::other(error)))?;
        find_account(&mut client, name).await
    }
}

#[async_trait]
impl CharacterRepository for SqlRepositories {
    async fn list_characters(
        &self,
        account: &str,
    ) -> Result<Vec<CharacterRecord>, tiberius::error::Error> {
        let mut client = self
            .pool
            .get()
            .await
            .map_err(|error| tiberius::error::Error::from(std::io::Error::other(error)))?;
        list_characters(&mut client, account).await
    }

    async fn belongs_to_account(
        &self,
        account: &str,
        character: &str,
    ) -> Result<bool, tiberius::error::Error> {
        let mut client = self
            .pool
            .get()
            .await
            .map_err(|error| tiberius::error::Error::from(std::io::Error::other(error)))?;
        character_belongs_to_account(&mut client, account, character).await
    }

    async fn save_position(
        &self,
        character: &str,
        map: MapId,
        position: MapPosition,
    ) -> Result<(), Box<dyn std::error::Error + Send + Sync>> {
        let mut client = self.pool.get().await?;
        client.simple_query("BEGIN TRANSACTION").await?;
        let result = client.execute(
            "UPDATE UserDB.dbo.CharacterInfo SET MapId = @P1, PosX = @P2, PosY = @P3, PosZ = @P4, StateVersion = StateVersion + 1, StateUpdatedAt = sysdatetime() WHERE Name = @P5",
            &[&map.0, &(position.x as i32), &(position.y as i32), &(position.z as i32), &character],
        ).await;
        match result {
            Ok(result) if result.rows_affected().iter().sum::<u64>() == 1 => {
                client.simple_query("COMMIT TRANSACTION").await?;
                Ok(())
            }
            Ok(_) => {
                let _ = client.simple_query("ROLLBACK TRANSACTION").await;
                Err("personagem nao encontrado".into())
            }
            Err(error) => {
                let _ = client.simple_query("ROLLBACK TRANSACTION").await;
                Err(error.into())
            }
        }
    }
}

#[async_trait]
impl NpcRepository for SqlRepositories {
    async fn list_for_map(&self, map: MapId) -> Result<Vec<NpcSnapshot>, tiberius::error::Error> {
        let mut client = self
            .pool
            .get()
            .await
            .map_err(|error| tiberius::error::Error::from(std::io::Error::other(error)))?;
        list_npcs_for_map(&mut client, map).await
    }
}

#[async_trait]
impl ShopRepository for SqlRepositories {
    async fn buy(
        &self,
        character: &str,
        npc: NpcId,
        item: i32,
        quantity: i32,
    ) -> Result<(), Box<dyn std::error::Error + Send + Sync>> {
        let mut client = self.pool.get().await?;
        buy_item(&mut client, character, npc, item, quantity).await
    }
}
