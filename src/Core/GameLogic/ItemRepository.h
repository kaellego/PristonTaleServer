#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Shared/datatypes.h"

// Forward declarations para desacoplar headers
class DatabaseManager;
struct Item;           // Supondo que a struct 'Item' esteja definida em outro lugar
struct DefinitionItem; // Supondo que a struct 'DefinitionItem' esteja definida em outro lugar

/**
 * @class ItemRepository
 * @brief Gerencia todas as definições de itens e a lógica de atualização de stats.
 *
 * Carrega as definições de 'ItemList' e 'ItemListOld' do banco de dados na inicialização.
 * Fornece métodos para encontrar definições de itens e para atualizar os stats de
 * um item existente com base nas mudanças de balanceamento do jogo, preservando
 * seu nível de "perfeição" relativo.
 */
class ItemRepository {
public:
    /**
     * @brief Construtor que injeta a dependência do DatabaseManager.
     * @param dbManager O gerenciador de banco de dados para carregar as definições.
     */
    explicit ItemRepository(DatabaseManager& dbManager);

    /**
     * @brief Encontra a definição de um item pelo seu código.
     * @param itemCode O código do item (ex: 16843008).
     * @return Ponteiro constante para a definição do item, ou nullptr se não for encontrado.
     */
    const DefinitionItem* findItemDef(int itemCode) const;

    /**
     * @brief Executa a lógica de atualização de stats para um item específico.
     * Compara os stats atuais do item com as definições antigas e novas para
     * ajustar os valores, mantendo os bônus de "perfeição".
     * @param item Uma referência para o item a ser atualizado. O item será modificado diretamente.
     * @return true se o item foi modificado, false caso contrário.
     */
    bool updateItemStats(Item& item) const;

private:
    /**
     * @brief Carrega as tabelas ItemList e ItemListOld do banco de dados para a memória.
     */
    void loadItemDefinitions();

    /**
     * @brief Encontra a definição antiga de um item pelo seu código.
     * @param itemCode O código do item.
     * @return Ponteiro constante para a definição antiga, ou nullptr se não for encontrado.
     */
    const DefinitionItem* findOldItemDef(int itemCode) const;

    /**
     * @brief Remove temporariamente os bônus de aging, mix, etc., de um item.
     * @param item O item a ser "limpo" para análise de seus stats base.
     */
    void stripAugments(Item& item) const;

    /**
     * @brief Reaplica os bônus de aging, mix, etc., a um item com stats base atualizados.
     * @param originalItem O item original para referência dos bônus.
     * @param updatedItem O item com stats base atualizados que receberá os bônus.
     */
    void applyAugments(const Item& originalItem, Item& updatedItem) const;

    /**
     * @brief Função de template genérica para ajustar um stat específico (int, short, float).
     * @return true se o valor do stat foi alterado.
     */
    template<typename T>
    bool adjustStat(const std::string& itemName, T& statValue, T newMin, T newMax, T oldMin, T oldMax) const;

    // Referência para o gerenciador de banco de dados. Não possui o objeto.
    DatabaseManager& m_dbManager;

    // Mapas que armazenam as definições de itens carregadas, otimizando o acesso.
    std::map<int, DefinitionItem> m_itemDefs;
    std::map<int, DefinitionItem> m_oldItemDefs;
};