#pragma once

#include <string>
#include <vector>
#include <optional>
#include "Shared/datatypes.h" // Inclui a definição completa das structs CharacterData e WarehouseData

struct CharacterData;
struct WarehouseData;

/**
 * @class PlayerRepository
 * @brief Gerencia a persistência (leitura e escrita) dos dados dos jogadores.
 */
class PlayerRepository {
public:
    /**
     * @brief Construtor que define o caminho base para a pasta de dados.
     * @param dataPath O caminho para a pasta 'Data', ex: "./Data".
     */
    explicit PlayerRepository(const std::string& dataPath);

    std::optional<CharacterData> loadCharacter(const std::string& characterName) const;
    bool saveCharacter(const CharacterData& characterData) const;
    std::optional<WarehouseData> loadWarehouse(const std::string& accountName) const;
    bool saveWarehouse(const std::string& accountName, const WarehouseData& warehouseData) const;
    std::vector<std::string> getAllCharacterNames() const;

private:
    std::string m_basePath;
    std::string m_charPath;
    std::string m_warehousePath;
};