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
 * @brief Gerencia todas as defini��es de itens e a l�gica de atualiza��o de stats.
 *
 * Carrega as defini��es de 'ItemList' e 'ItemListOld' do banco de dados na inicializa��o.
 * Fornece m�todos para encontrar defini��es de itens e para atualizar os stats de
 * um item existente com base nas mudan�as de balanceamento do jogo, preservando
 * seu n�vel de "perfei��o" relativo.
 */
class ItemRepository {
public:
    /**
     * @brief Construtor que injeta a depend�ncia do DatabaseManager.
     * @param dbManager O gerenciador de banco de dados para carregar as defini��es.
     */
    explicit ItemRepository(DatabaseManager& dbManager);

    /**
     * @brief Encontra a defini��o de um item pelo seu c�digo.
     * @param itemCode O c�digo do item (ex: 16843008).
     * @return Ponteiro constante para a defini��o do item, ou nullptr se n�o for encontrado.
     */
    const DefinitionItem* findItemDef(int itemCode) const;

    /**
     * @brief Executa a l�gica de atualiza��o de stats para um item espec�fico.
     * Compara os stats atuais do item com as defini��es antigas e novas para
     * ajustar os valores, mantendo os b�nus de "perfei��o".
     * @param item Uma refer�ncia para o item a ser atualizado. O item ser� modificado diretamente.
     * @return true se o item foi modificado, false caso contr�rio.
     */
    bool updateItemStats(Item& item) const;

private:
    /**
     * @brief Carrega as tabelas ItemList e ItemListOld do banco de dados para a mem�ria.
     */
    void loadItemDefinitions();

    /**
     * @brief Encontra a defini��o antiga de um item pelo seu c�digo.
     * @param itemCode O c�digo do item.
     * @return Ponteiro constante para a defini��o antiga, ou nullptr se n�o for encontrado.
     */
    const DefinitionItem* findOldItemDef(int itemCode) const;

    /**
     * @brief Remove temporariamente os b�nus de aging, mix, etc., de um item.
     * @param item O item a ser "limpo" para an�lise de seus stats base.
     */
    void stripAugments(Item& item) const;

    /**
     * @brief Reaplica os b�nus de aging, mix, etc., a um item com stats base atualizados.
     * @param originalItem O item original para refer�ncia dos b�nus.
     * @param updatedItem O item com stats base atualizados que receber� os b�nus.
     */
    void applyAugments(const Item& originalItem, Item& updatedItem) const;

    /**
     * @brief Fun��o de template gen�rica para ajustar um stat espec�fico (int, short, float).
     * @return true se o valor do stat foi alterado.
     */
    template<typename T>
    bool adjustStat(const std::string& itemName, T& statValue, T newMin, T newMax, T oldMin, T oldMax) const;

    // Refer�ncia para o gerenciador de banco de dados. N�o possui o objeto.
    DatabaseManager& m_dbManager;

    // Mapas que armazenam as defini��es de itens carregadas, otimizando o acesso.
    std::map<int, DefinitionItem> m_itemDefs;
    std::map<int, DefinitionItem> m_oldItemDefs;
};