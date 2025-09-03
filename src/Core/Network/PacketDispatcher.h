#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <functional>

// Forward declarations
class ClientSession;
struct Packet;
struct GlobalState;
class PlayerRepository;
class ItemRepository;

class IPacketHandler {
public:
    virtual ~IPacketHandler() = default;
    virtual void handle(std::shared_ptr<ClientSession> session, const Packet& packet) = 0;
};

class PacketDispatcher {
public:
    PacketDispatcher(GlobalState& state, PlayerRepository& playerRepo, ItemRepository& itemRepo);
    void dispatch(std::shared_ptr<ClientSession> session, const Packet& packet);

private:
    void registerHandlers();

    GlobalState& m_globalState;
    PlayerRepository& m_playerRepository;
    ItemRepository& m_itemRepository;

    std::map<uint16_t, std::unique_ptr<IPacketHandler>> m_handlers;
};