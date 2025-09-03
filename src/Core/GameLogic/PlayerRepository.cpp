#include "GameLogic/PlayerRepository.h"
#include "Shared/datatypes.h"
#include <fstream>
#include <filesystem> // Biblioteca padrão do C++ para manipulação de arquivos
#include <iostream>

namespace fs = std::filesystem;

PlayerRepository::PlayerRepository(const std::string& dataPath) : m_basePath(dataPath) {
    // Monta os caminhos completos para as pastas de personagem e depósito
    m_charPath = (fs::path(m_basePath) / "Character").string(); // Adicione .string()
    m_warehousePath = (fs::path(m_basePath) / "Warehouse").string(); // Adicione .string()

    std::cout << "Inicializando repositorio de jogadores..." << std::endl;
    std::cout << "  - Caminho dos Personagens: " << m_charPath << std::endl;
    std::cout << "  - Caminho dos Depositos: " << m_warehousePath << std::endl;
}

std::optional<CharacterData> PlayerRepository::loadCharacter(const std::string& characterName) const {
    fs::path filePath = fs::path(m_charPath) / (characterName + ".chr");

    if (!fs::exists(filePath)) {
        return std::nullopt; // Retorna vazio se o arquivo não existe
    }

    // Abre o arquivo para leitura em modo binário
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "ERRO: Nao foi possivel abrir o arquivo do personagem: " << filePath << std::endl;
        return std::nullopt;
    }

    // Obtém o tamanho do arquivo e volta para o início
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    CharacterData charData;
    // O código original pode ter lógicas complexas de leitura, aqui simplificamos para leitura direta.
    // Você precisará adaptar esta parte para corresponder à estrutura exata do seu arquivo .chr.
    if (file.read(reinterpret_cast<char*>(&charData), size)) {
        // TODO: Adicionar aqui qualquer lógica de descriptografia ou validação necessária
        return charData;
    }

    std::cerr << "ERRO: Falha ao ler os dados do arquivo do personagem: " << filePath << std::endl;
    return std::nullopt;
}

bool PlayerRepository::saveCharacter(const CharacterData& characterData) const {
    // O nome do arquivo é obtido diretamente da struct de dados.
    fs::path filePath = fs::path(m_charPath) / (std::string(characterData.szName) + ".chr");

    // Abre o arquivo para escrita em modo binário, truncando o conteúdo existente.
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "ERRO: Nao foi possivel abrir/criar o arquivo para salvar o personagem: " << filePath << std::endl;
        return false;
    }

    // TODO: Adicionar aqui qualquer lógica de criptografia necessária antes de salvar.

    // Escreve todo o bloco de memória da struct para o arquivo.
    // Cuidado: isso só funciona se a struct for 'Plain Old Data' (POD).
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
    // Semelhante ao personagem, adapte a lógica de leitura à estrutura do seu arquivo .dat.
    if (file.read(reinterpret_cast<char*>(&warehouseData), sizeof(WarehouseData))) {
        // TODO: Lógica de descriptografia (ex: WAREHOUSEHANDLER->Decrypt)
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

    // TODO: Lógica de criptografia (ex: WAREHOUSEHANDLER->Encrypt)

    file.write(reinterpret_cast<const char*>(&warehouseData), sizeof(WarehouseData));

    return file.good();
}

std::vector<std::string> PlayerRepository::getAllCharacterNames() const {
    std::vector<std::string> names;

    if (!fs::exists(m_charPath) || !fs::is_directory(m_charPath)) {
        std::cerr << "AVISO: Diretorio de personagens nao encontrado: " << m_charPath << std::endl;
        return names;
    }

    // Itera sobre todos os arquivos no diretório de personagens.
    for (const auto& entry : fs::directory_iterator(m_charPath)) {
        // Verifica se é um arquivo regular e se a extensão é ".chr"
        if (entry.is_regular_file() && entry.path().extension() == ".chr") {
            // Adiciona o nome do arquivo sem a extensão à lista.
            names.push_back(entry.path().stem().string());
        }
    }

    return names;
}