#pragma once

#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <memory>
#include "GameLogic/CharacterFileRepository.h"
#include "GameLogic/StorageFileRepository.h"
#include "GameLogic/PlayerStateSqlRepository.h"
#include "Shared/datatypes.h" // Inclui a definiÃ§Ã£o completa das structs CharacterData e WarehouseData

struct CharacterData;
struct WarehouseData;
class DatabasePool;

/**
 * @class PlayerRepository
 * @brief Gerencia a persistÃªncia (leitura e escrita) dos dados dos jogadores.
 */
class PlayerRepository {
public:
    /**
     * @brief Construtor que define o caminho base para a pasta de dados.
     * @param dataPath O caminho para a pasta 'Data', ex: "./Data".
     */
    explicit PlayerRepository(const std::string& dataPath, DatabasePool* dbPool = nullptr);

    std::optional<CharacterData> loadCharacter(const std::string& characterName) const;
    bool characterFileExists(const std::string& characterName) const;
    bool saveCharacter(const CharacterData& characterData) const;
    bool deleteCharacter(const std::string& characterName) const;
    std::optional<WarehouseData> loadWarehouse(const std::string& accountName) const;
    bool saveWarehouse(const std::string& accountName, const WarehouseData& warehouseData) const;
    std::optional<InventoryData> loadInventory(const std::string& characterName) const;
    bool saveInventory(const std::string& characterName, const InventoryData& inventoryData) const;
    bool deleteInventory(const std::string& characterName) const;
    bool deleteWarehouse(const std::string& accountName) const;
    std::vector<std::string> getAllCharacterNames() const;

private:
    CharacterFileRepository m_characters;
    StorageFileRepository m_storage;
    std::unique_ptr<PlayerStateSqlRepository> m_sql;
};
