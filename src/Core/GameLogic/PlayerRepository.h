#pragma once

#include <string>
#include <vector>
#include <optional> // Para retornar valores que podem n�o existir

// Forward declarations para as estruturas de dados principais.
// Supondo que estejam definidas em um arquivo como "datatypes.h".
struct CharacterData;
struct WarehouseData;

/**
 * @class PlayerRepository
 * @brief Gerencia a persist�ncia (leitura e escrita) dos dados dos jogadores.
 *
 * Abstrai o acesso ao sistema de arquivos para carregar e salvar personagens (.chr)
 * e dep�sitos (.dat). Utiliza a biblioteca <filesystem> do C++ para
 * opera��es de diret�rio de forma moderna e port�vel.
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
     * @param characterName O nome do personagem (sem a extens�o .chr).
     * @return Um std::optional<CharacterData> contendo os dados do personagem se o
     * arquivo for lido com sucesso; caso contr�rio, um optional vazio.
     */
    std::optional<CharacterData> loadCharacter(const std::string& characterName) const;

    /**
     * @brief Salva os dados de um personagem em um arquivo .chr.
     * @param characterData Os dados do personagem a serem salvos.
     * @return true se a opera��o for bem-sucedida, false caso contr�rio.
     */
    bool saveCharacter(const CharacterData& characterData) const;

    /**
     * @brief Carrega os dados de um dep�sito de um arquivo .dat.
     * @param accountName O nome da conta (sem a extens�o .dat).
     * @return Um std::optional<WarehouseData> contendo os dados do dep�sito se
     * o arquivo for lido com sucesso; caso contr�rio, um optional vazio.
     */
    std::optional<WarehouseData> loadWarehouse(const std::string& accountName) const;

    /**
     * @brief Salva os dados de um dep�sito em um arquivo .dat.
     * @param accountName O nome da conta para nomear o arquivo.
     * @param warehouseData Os dados do dep�sito a serem salvos.
     * @return true se a opera��o for bem-sucedida, false caso contr�rio.
     */
    bool saveWarehouse(const std::string& accountName, const WarehouseData& warehouseData) const;

    /**
     * @brief Obt�m uma lista com os nomes de todos os personagens na pasta de dados.
     * @return Um vetor de strings, onde cada string � um nome de personagem.
     */
    std::vector<std::string> getAllCharacterNames() const;

private:
    // Caminho base para a pasta 'Data' do servidor.
    std::string m_basePath;
    std::string m_charPath;
    std::string m_warehousePath;
};