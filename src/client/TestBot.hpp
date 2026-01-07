#pragma once

#include "../network/Socket.hpp"
#include "../proto/messages.pb.h"
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <functional>
#include <queue>
#include <chrono>

namespace mmorpg::client {

namespace asio = boost::asio;

// Combat statistics tracking
struct CombatStats {
    int totalDamageDealt = 0;
    int totalDamageReceived = 0;
    int skillsUsed = 0;
    int attacksLanded = 0;
    int attacksDodged = 0;
    int criticalHits = 0;
    int healsPerformed = 0;
    int totalHealing = 0;
};

// Simple test bot client
class TestBot {
public:
    using PacketHandler = std::function<void(const proto::Packet&)>;

    TestBot(const std::string& name);
    ~TestBot();

    // Connection
    bool connect(const std::string& host, uint16_t port);
    void disconnect();
    bool isConnected() const { return socket_ && socket_->isValid() && connected_; }

    // Login
    bool login(const std::string& password = "");

    // Network operations
    bool sendPacket(proto::MessageType type, const google::protobuf::Message& message);
    bool poll(int timeoutMs = 100);

    // Game actions
    bool attack(uint32_t targetId);
    bool useSkill(uint32_t skillId, uint32_t targetId = 0);
    bool learnSkill(uint32_t skillId);
    bool upgradeSkill(uint32_t skillId);
    bool chat(const std::string& message);
    bool ping();

    // Accessors
    const std::string& getName() const { return name_; }
    uint32_t getActorId() const { return actorId_; }
    const proto::ActorInfo& getActorInfo() const { return actorInfo_; }
    const proto::SkillList& getSkillList() const { return skillList_; }
    const std::vector<proto::ActorInfo>& getOtherActors() const { return otherActors_; }

    // Set packet handler for custom processing
    void setPacketHandler(PacketHandler handler) { packetHandler_ = std::move(handler); }

    // Get last received packets
    std::vector<proto::Packet>& getReceivedPackets() { return receivedPackets_; }
    void clearReceivedPackets() { receivedPackets_.clear(); }

    // Reconnection support
    bool reconnect();
    bool reconnectWithDelay(int delayMs);

    // Combat statistics
    const CombatStats& getCombatStats() const { return combatStats_; }
    void resetCombatStats() { combatStats_ = CombatStats{}; }

    // Utility methods
    bool waitForPackets(int count, int timeoutMs = 1000);
    bool pollUntil(std::function<bool()> condition, int timeoutMs = 5000, int pollIntervalMs = 50);

private:
    bool receiveAndProcess();
    void handlePacket(const proto::Packet& packet);

    std::string name_;
    asio::io_context io_;
    std::unique_ptr<net::Socket> socket_;
    bool connected_ = false;
    uint32_t actorId_ = 0;

    // Receive buffer
    std::vector<uint8_t> recvBuffer_;

    // State
    proto::ActorInfo actorInfo_;
    proto::SkillList skillList_;
    std::vector<proto::ActorInfo> otherActors_;

    // Packet handler
    PacketHandler packetHandler_;
    std::vector<proto::Packet> receivedPackets_;

    // Reconnection support
    std::string lastHost_;
    uint16_t lastPort_ = 0;

    // Combat statistics
    CombatStats combatStats_;
};

} // namespace mmorpg::client
