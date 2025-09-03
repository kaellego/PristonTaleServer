#include "GameLogic/PlayerRepository.h"
#include "Shared/datatypes.h"
#include <fstream>
#include <filesystem> // Biblioteca padr�o do C++ para manipula��o de arquivos
#include <iostream>

namespace fs = std::filesystem;

PlayerRepository::PlayerRepository(const std::string& dataPath) : m_basePath(dataPath) {
    // Monta os caminhos completos para as pastas de personagem e dep�sito
    m_charPath = (fs::path(m_basePath) / "Character").string(); // Adicione .string()
    m_warehousePath = (fs::path(m_basePath) / "Warehouse").string(); // Adicione .string()

    std::cout << "Inicializando repositorio de jogadores..." << std::endl;
    std::cout << "  - Caminho dos Personagens: " << m_charPath << std::endl;
    std::cout << "  - Caminho dos Depositos: " << m_warehousePath << std::endl;
}

std::optional<CharacterData> PlayerRepository::loadCharacter(const std::string& characterName) const {
    fs::path filePath = fs::path(m_charPath) / (characterName + ".chr");

    if (!fs::exists(filePath)) {
        return std::nullopt; // Retorna vazio se o arquivo n�o existe
    }

    // Abre o arquivo para leitura em modo bin�rio
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "ERRO: Nao foi possivel abrir o arquivo do personagem: " << filePath << std::endl;
        return std::nullopt;
    }

    // Obt�m o tamanho do arquivo e volta para o in�cio
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    CharacterData charData;
    // O c�digo original pode ter l�gicas complexas de leitura, aqui simplificamos para leitura direta.
    // Voc� precisar� adaptar esta parte para corresponder � estrutura exata do seu arquivo .chr.
    if (file.read(reinterpret_cast<char*>(&charData), size)) {
        // TODO: Adicionar aqui qualquer l�gica de descriptografia ou valida��o necess�ria
        return charData;
    }

    std::cerr << "ERRO: Falha ao ler os dados do arquivo do personagem: " << filePath << std::endl;
    return std::nullopt;
}

bool PlayerRepository::saveCharacter(const CharacterData& characterData) const {
    // O nome do arquivo � obtido diretamente da struct de dados.
    fs::path filePath = fs::path(m_charPath) / (std::string(characterData.szName) + ".chr");

    // Abre o arquivo para escrita em modo bin�rio, truncando o conte�do existente.
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "ERRO: Nao foi possivel abrir/criar o arquivo para salvar o personagem: " << filePath << std::endl;
        return false;
    }

    // TODO: Adicionar aqui qualquer l�gica de criptografia necess�ria antes de salvar.

    // Escreve todo o bloco de mem�ria da struct para o arquivo.
    // Cuidado: isso s� funciona se a struct for 'Plain Old Data' (POD).
    file.write(reinterpret_cast<const char*>(&characterData), sizeof(CharacterData));

    return file.good(); // Retorna true se a escrita ocorreu sem erros.
}

std::optional<WarehouseData> PlayerRepository::loadWarehouse(const std::string& accountName) const {
    fs::path filePath = fs::path(m_warehousePath) / (accountName + ".dat");

    if (!fs::exists(filePath)) {
        return std::nullopt;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERRO: Nao foi possivel abrir o arquivo do deposito: " << filePath << std::endl;
        return std::nullopt;
    }

    WarehouseData warehouseData;
    // Semelhante ao personagem, adapte a l�gica de leitura � estrutura do seu arquivo .dat.
    if (file.read(reinterpret_cast<char*>(&warehouseData), sizeof(WarehouseData))) {
        // TODO: L�gica de descriptografia (ex: WAREHOUSEHANDLER->Decrypt)
        return warehouseData;
    }

    std::cerr << "ERRO: Falha ao ler os dados do arquivo do deposito: " << filePath << std::endl;
    return std::nullopt;
}

bool PlayerRepository::saveWarehouse(const std::string& accountName, const WarehouseData& warehouseData) const {
    fs::path filePath = fs::path(m_warehousePath) / (accountName + ".dat");
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
        std::cerr << "ERRO: Nao foi possivel abrir/criar o arquivo para salvar o deposito: " << filePath << std::endl;
        return false;
    }

    // TODO: L�gica de criptografia (ex: WAREHOUSEHANDLER->Encrypt)

    file.write(reinterpret_cast<const char*>(&warehouseData), sizeof(WarehouseData));

    return file.good();
}

std::vector<std::string> PlayerRepository::getAllCharacterNames() const {
    std::vector<std::string> names;

    if (!fs::exists(m_charPath) || !fs::is_directory(m_charPath)) {
        std::cerr << "AVISO: Diretorio de personagens nao encontrado: " << m_charPath << std::endl;
        return names;
    }

    // Itera sobre todos os arquivos no diret�rio de personagens.
    for (const auto& entry : fs::directory_iterator(m_charPath)) {
        // Verifica se � um arquivo regular e se a extens�o � ".chr"
        if (entry.is_regular_file() && entry.path().extension() == ".chr") {
            // Adiciona o nome do arquivo sem a extens�o � lista.
            names.push_back(entry.path().stem().string());
        }
    }

    return names;
}