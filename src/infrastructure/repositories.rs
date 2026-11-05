use super::sql::SqlClient;
use crate::domain::{AccountId, CharacterId, MapId, MapPosition, NpcId, NpcSnapshot};
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

pub async fn find_account(
    client: &mut SqlClient,
    name: &str,
) -> Result<Option<AccountRecord>, tiberius::error::Error> {
    let mut rows = client
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
    let mut rows = client
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

pub async fn list_npcs_for_map(
    client: &mut SqlClient,
    map: MapId,
) -> Result<Vec<NpcSnapshot>, tiberius::error::Error> {
    let mut rows = client
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
