#pragma once

#include <string>
#include <vector>
#include <optional> // Para retornar valores que podem não existir

// Forward declarations para as estruturas de dados principais.
// Supondo que estejam definidas em um arquivo como "datatypes.h".
struct CharacterData;
struct WarehouseData;

/**
 * @class PlayerRepository
 * @brief Gerencia a persistência (leitura e escrita) dos dados dos jogadores.
 *
 * Abstrai o acesso ao sistema de arquivos para carregar e salvar personagens (.chr)
 * e depósitos (.dat). Utiliza a biblioteca <filesystem> do C++ para
 * operações de diretório de forma moderna e portável.
 */
class PlayerRepository {
public:
    /**
     * @brief Construtor que define o caminho base para a pasta de dados.
     * @param dataPath O caminho para a pasta 'Data', ex: "C:/Server/Data/".
     */
    explicit PlayerRepository(const std::string& dataPath);

    /**
     * @brief Carrega os dados de um personagem de um arquivo .chr.
     * @param characterName O nome do personagem (sem a extensão .chr).
     * @return Um std::optional<CharacterData> contendo os dados do personagem se o
     * arquivo for lido com sucesso; caso contrário, um optional vazio.
     */
    std::optional<CharacterData> loadCharacter(const std::string& characterName) const;

    /**
     * @brief Salva os dados de um personagem em um arquivo .chr.
     * @param characterData Os dados do personagem a serem salvos.
     * @return true se a operação for bem-sucedida, false caso contrário.
     */
    bool saveCharacter(const CharacterData& characterData) const;

    /**
     * @brief Carrega os dados de um depósito de um arquivo .dat.
     * @param accountName O nome da conta (sem a extensão .dat).
     * @return Um std::optional<WarehouseData> contendo os dados do depósito se
     * o arquivo for lido com sucesso; caso contrário, um optional vazio.
     */
    std::optional<WarehouseData> loadWarehouse(const std::string& accountName) const;

    /**
     * @brief Salva os dados de um depósito em um arquivo .dat.
     * @param accountName O nome da conta para nomear o arquivo.
     * @param warehouseData Os dados do depósito a serem salvos.
     * @return true se a operação for bem-sucedida, false caso contrário.
     */
    bool saveWarehouse(const std::string& accountName, const WarehouseData& warehouseData) const;

    /**
     * @brief Obtém uma lista com os nomes de todos os personagens na pasta de dados.
     * @return Um vetor de strings, onde cada string é um nome de personagem.
     */
    std::vector<std::string> getAllCharacterNames() const;

private:
    // Caminho base para a pasta 'Data' do servidor.
    std::string m_basePath;
    std::string m_charPath;
    std::string m_warehousePath;
};