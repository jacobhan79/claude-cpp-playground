#include "TestBot.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <boost/endian/conversion.hpp>

namespace mmorpg::client {

TestBot::TestBot(const std::string& name) : name_(name) {
    recvBuffer_.reserve(65536);
}

TestBot::~TestBot() {
    disconnect();
}

bool TestBot::connect(const std::string& host, uint16_t port) {
    socket_ = std::make_unique<net::Socket>(io_);

    if (!socket_->connect(host, port)) {
        std::cerr << "[" << name_ << "] Failed to connect to " << host << ":" << port << std::endl;
        return false;
    }

    socket_->setNoDelay(true);
    socket_->setNonBlocking(true);
    connected_ = true;
    lastHost_ = host;
    lastPort_ = port;
    std::cout << "[" << name_ << "] Connected to server" << std::endl;
    return true;
}

void TestBot::disconnect() {
    if (connected_) {
        if (socket_) {
            socket_->close();
        }
        connected_ = false;
        std::cout << "[" << name_ << "] Disconnected" << std::endl;
    }
}

bool TestBot::login(const std::string& password) {
    proto::LoginRequest req;
    req.set_username(name_);
    req.set_password(password);
    return sendPacket(proto::MSG_LOGIN_REQUEST, req);
}

bool TestBot::sendPacket(proto::MessageType type, const google::protobuf::Message& message) {
    if (!connected_ || !socket_) return false;

    proto::Packet packet;
    packet.set_type(static_cast<uint32_t>(type));
    packet.set_payload(message.SerializeAsString());

    std::string data = packet.SerializeAsString();
    uint32_t len = boost::endian::native_to_big(static_cast<uint32_t>(data.size()));

    // Send length prefix
    if (!socket_->send(reinterpret_cast<uint8_t*>(&len), 4)) {
        connected_ = false;
        return false;
    }

    // Send data
    if (!socket_->send(reinterpret_cast<const uint8_t*>(data.data()), data.size())) {
        connected_ = false;
        return false;
    }

    return true;
}

bool TestBot::poll(int timeoutMs) {
    if (!connected_ || !socket_) return false;

    // Use Boost.Asio for polling
    boost::system::error_code ec;
    socket_->getAsioSocket().wait(asio::socket_base::wait_read, ec);

    if (!ec) {
        return receiveAndProcess();
    }

    // EAGAIN/EWOULDBLOCK is not an error for non-blocking sockets
    if (ec == asio::error::would_block || ec == asio::error::try_again) {
        return true;
    }

    return !ec;
}

bool TestBot::receiveAndProcess() {
    uint8_t buffer[4096];
    int received = socket_->receiveInto(buffer, sizeof(buffer));

    if (received < 0) {
        // For non-blocking, no data is not an error
        return true;
    }

    if (received == 0) {
        connected_ = false;
        return false;
    }

    recvBuffer_.insert(recvBuffer_.end(), buffer, buffer + received);

    // Parse packets
    while (recvBuffer_.size() >= 4) {
        uint32_t packetLen;
        memcpy(&packetLen, recvBuffer_.data(), 4);
        packetLen = boost::endian::big_to_native(packetLen);

        if (packetLen > 1024 * 1024) {
            std::cerr << "[" << name_ << "] Packet too large" << std::endl;
            connected_ = false;
            return false;
        }

        if (recvBuffer_.size() < 4 + packetLen) {
            break;
        }

        proto::Packet packet;
        if (packet.ParseFromArray(recvBuffer_.data() + 4, packetLen)) {
            handlePacket(packet);
            receivedPackets_.push_back(packet);
        }

        recvBuffer_.erase(recvBuffer_.begin(), recvBuffer_.begin() + 4 + packetLen);
    }

    return true;
}

void TestBot::handlePacket(const proto::Packet& packet) {
    auto type = static_cast<proto::MessageType>(packet.type());

    switch (type) {
        case proto::MSG_LOGIN_RESPONSE: {
            proto::LoginResponse resp;
            if (resp.ParseFromString(packet.payload())) {
                if (resp.success()) {
                    actorId_ = resp.actor_id();
                    actorInfo_ = resp.actor();
                    std::cout << "[" << name_ << "] Login successful! Actor ID: " << actorId_ << std::endl;
                    std::cout << "[" << name_ << "] HP: " << actorInfo_.current_hp()
                              << "/" << actorInfo_.max_hp()
                              << " MP: " << actorInfo_.current_mp()
                              << "/" << actorInfo_.max_mp() << std::endl;
                } else {
                    std::cout << "[" << name_ << "] Login failed: " << resp.message() << std::endl;
                }
            }
            break;
        }
        case proto::MSG_ACTOR_SPAWN: {
            proto::ActorSpawn spawn;
            if (spawn.ParseFromString(packet.payload())) {
                otherActors_.push_back(spawn.actor());
                std::cout << "[" << name_ << "] Player joined: " << spawn.actor().name()
                          << " (ID: " << spawn.actor().id() << ")" << std::endl;
            }
            break;
        }
        case proto::MSG_ACTOR_DESPAWN: {
            proto::ActorDespawn despawn;
            if (despawn.ParseFromString(packet.payload())) {
                otherActors_.erase(
                    std::remove_if(otherActors_.begin(), otherActors_.end(),
                        [&](const proto::ActorInfo& a) { return a.id() == despawn.actor_id(); }),
                    otherActors_.end()
                );
                std::cout << "[" << name_ << "] Player left (ID: " << despawn.actor_id() << ")" << std::endl;
            }
            break;
        }
        case proto::MSG_ACTOR_LIST: {
            proto::ActorList list;
            if (list.ParseFromString(packet.payload())) {
                for (const auto& actor : list.actors()) {
                    otherActors_.push_back(actor);
                    std::cout << "[" << name_ << "] Existing player: " << actor.name()
                              << " (ID: " << actor.id() << ")" << std::endl;
                }
            }
            break;
        }
        case proto::MSG_ATTACK_RESULT: {
            proto::AttackResult result;
            if (result.ParseFromString(packet.payload())) {
                if (result.is_dodged()) {
                    std::cout << "[" << name_ << "] Attack dodged!" << std::endl;
                    // Track dodged attacks
                    if (result.target_id() == actorId_) {
                        combatStats_.attacksDodged++;
                    }
                } else {
                    std::cout << "[" << name_ << "] Attack: " << result.attacker_id()
                              << " -> " << result.target_id()
                              << " for " << result.damage() << " damage"
                              << (result.is_critical() ? " (CRIT!)" : "")
                              << " [HP: " << result.target_hp() << "]" << std::endl;

                    // Track combat statistics
                    if (result.attacker_id() == actorId_) {
                        combatStats_.totalDamageDealt += result.damage();
                        combatStats_.attacksLanded++;
                        if (result.is_critical()) {
                            combatStats_.criticalHits++;
                        }
                    }
                    if (result.target_id() == actorId_) {
                        combatStats_.totalDamageReceived += result.damage();
                    }
                }

                // Update our HP if we were the target
                if (result.target_id() == actorId_) {
                    actorInfo_.set_current_hp(result.target_hp());
                }
            }
            break;
        }
        case proto::MSG_SKILL_RESULT: {
            proto::SkillResult result;
            if (result.ParseFromString(packet.payload())) {
                std::cout << "[" << name_ << "] Skill " << result.skill_id()
                          << ": " << result.message() << std::endl;

                // Track skill usage statistics
                if (result.success() && result.caster_id() == actorId_) {
                    combatStats_.skillsUsed++;
                    if (result.damage() > 0) {
                        combatStats_.totalDamageDealt += result.damage();
                    }
                    if (result.heal() > 0) {
                        combatStats_.healsPerformed++;
                        combatStats_.totalHealing += result.heal();
                    }
                }
            }
            break;
        }
        case proto::MSG_SKILL_LIST: {
            proto::SkillList list;
            if (list.ParseFromString(packet.payload())) {
                skillList_ = list;
                std::cout << "[" << name_ << "] Skill points: " << list.skill_points()
                          << ", Skills: " << list.skills_size() << std::endl;
            }
            break;
        }
        case proto::MSG_CHAT: {
            proto::Chat chat;
            if (chat.ParseFromString(packet.payload())) {
                std::cout << "[CHAT] " << chat.sender_name() << ": " << chat.message() << std::endl;
            }
            break;
        }
        case proto::MSG_PONG: {
            proto::Pong pong;
            if (pong.ParseFromString(packet.payload())) {
                auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                std::cout << "[" << name_ << "] Pong! Latency: "
                          << (now - pong.timestamp()) << "ms" << std::endl;
            }
            break;
        }
        case proto::MSG_ERROR: {
            proto::Error error;
            if (error.ParseFromString(packet.payload())) {
                std::cout << "[" << name_ << "] Error " << error.code()
                          << ": " << error.message() << std::endl;
            }
            break;
        }
        default:
            break;
    }

    // Call custom handler if set
    if (packetHandler_) {
        packetHandler_(packet);
    }
}

bool TestBot::attack(uint32_t targetId) {
    proto::AttackRequest req;
    req.set_target_id(targetId);
    return sendPacket(proto::MSG_ATTACK_REQUEST, req);
}

bool TestBot::useSkill(uint32_t skillId, uint32_t targetId) {
    proto::SkillRequest req;
    req.set_skill_id(skillId);
    req.set_target_id(targetId);
    return sendPacket(proto::MSG_SKILL_REQUEST, req);
}

bool TestBot::learnSkill(uint32_t skillId) {
    proto::LearnSkill req;
    req.set_skill_id(skillId);
    return sendPacket(proto::MSG_LEARN_SKILL, req);
}

bool TestBot::upgradeSkill(uint32_t skillId) {
    proto::UpgradeSkill req;
    req.set_skill_id(skillId);
    return sendPacket(proto::MSG_UPGRADE_SKILL, req);
}

bool TestBot::chat(const std::string& message) {
    proto::Chat chatMsg;
    chatMsg.set_message(message);
    return sendPacket(proto::MSG_CHAT, chatMsg);
}

bool TestBot::ping() {
    proto::Ping pingMsg;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    pingMsg.set_timestamp(now);
    return sendPacket(proto::MSG_PING, pingMsg);
}

bool TestBot::reconnect() {
    if (lastHost_.empty() || lastPort_ == 0) {
        std::cerr << "[" << name_ << "] No previous connection to reconnect to" << std::endl;
        return false;
    }

    // Disconnect if still connected
    if (connected_) {
        disconnect();
    }

    // Clear state for fresh reconnection
    actorId_ = 0;
    otherActors_.clear();
    recvBuffer_.clear();
    receivedPackets_.clear();

    return connect(lastHost_, lastPort_);
}

bool TestBot::reconnectWithDelay(int delayMs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    return reconnect();
}

bool TestBot::waitForPackets(int count, int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    size_t initialCount = receivedPackets_.size();

    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(timeoutMs)) {
        poll(50);
        if (receivedPackets_.size() >= initialCount + count) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return receivedPackets_.size() >= initialCount + count;
}

bool TestBot::pollUntil(std::function<bool()> condition, int timeoutMs, int pollIntervalMs) {
    auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(timeoutMs)) {
        poll(pollIntervalMs);
        if (condition()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
    }

    return condition();
}

} // namespace mmorpg::client
