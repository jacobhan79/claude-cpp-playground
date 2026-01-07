#pragma once

#include "../network/TcpServer.hpp"
#include "../actors/Character.hpp"
#include "../actors/ActorManager.hpp"
#include "../combat/CombatSystem.hpp"
#include "../core/EventBus.hpp"
#include "../skills/SkillTree.hpp"
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/container/flat_map.hpp>
#include <memory>
#include <string>

namespace mmorpg {

namespace asio = boost::asio;
namespace pt = boost::property_tree;

class GameServer {
public:
    struct Config {
        // Server settings
        uint16_t port = 7777;
        uint32_t tickRate = 20;  // Ticks per second

        // Network settings
        uint32_t maxConnections = 100;
        uint32_t timeoutMs = 30000;

        // Game settings
        int32_t startingLevel = 1;
        int32_t startingSkillPoints = 3;
        float expMultiplier = 1.0f;

        // Load config from JSON file
        static Config loadFromFile(const std::string& filename);
    };

    explicit GameServer(Config config);
    ~GameServer();

    // Lifecycle
    bool initialize();
    void run();
    void shutdown();
    bool isRunning() const { return running_; }

    // Access to subsystems (for testing)
    ActorManager& getActorManager() { return *actorManager_; }
    CombatSystem& getCombatSystem() { return *combatSystem_; }
    EventBus& getEventBus() { return *eventBus_; }

private:
    // Packet handlers
    void handlePacket(net::ConnectionPtr conn, const proto::Packet& packet);
    void handleLogin(net::ConnectionPtr conn, const proto::LoginRequest& req);
    void handleLogout(net::ConnectionPtr conn);
    void handleAttack(net::ConnectionPtr conn, const proto::AttackRequest& req);
    void handleSkillRequest(net::ConnectionPtr conn, const proto::SkillRequest& req);
    void handleLearnSkill(net::ConnectionPtr conn, const proto::LearnSkill& req);
    void handleUpgradeSkill(net::ConnectionPtr conn, const proto::UpgradeSkill& req);
    void handleChat(net::ConnectionPtr conn, const proto::Chat& chat);
    void handlePing(net::ConnectionPtr conn, const proto::Ping& ping);

    // Connection events
    void onConnect(net::ConnectionPtr conn);
    void onDisconnect(net::ConnectionPtr conn);

    // Game events
    void onDamageEvent(const DamageEvent& event);
    void onDeathEvent(const DeathEvent& event);

    // Game loop
    void tick();

    // Helper to build ActorInfo proto
    proto::ActorInfo buildActorInfo(const Character& character);

    // Helper to build SkillList proto
    proto::SkillList buildSkillList(const Character& character);

    // Create default skill tree
    void setupSkillTree();

    Config config_;
    bool running_ = false;

    // Core systems
    std::shared_ptr<EventBus> eventBus_;
    std::unique_ptr<ActorManager> actorManager_;
    std::unique_ptr<CombatSystem> combatSystem_;
    std::unique_ptr<net::TcpServer> server_;

    // Mapping connection to character
    boost::container::flat_map<net::Connection::ConnectionId, std::shared_ptr<Character>> connToCharacter_;

    // Skill tree template
    SkillTree skillTree_;

    // Tick counter
    Tick currentTick_ = 0;

    // Timer for game tick (uses TcpServer's io_context)
    std::unique_ptr<asio::steady_timer> tickTimer_;
    std::chrono::milliseconds tickInterval_;

    // Async tick handler
    void scheduleNextTick();
    void onTickTimer(const boost::system::error_code& ec);

    // Event subscriptions
    EventBus::HandlerId damageEventId_ = 0;
    EventBus::HandlerId deathEventId_ = 0;
};

} // namespace mmorpg
