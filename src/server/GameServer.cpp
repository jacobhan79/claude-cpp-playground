#include "GameServer.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace mmorpg {

GameServer::Config GameServer::Config::loadFromFile(const std::string& filename) {
    Config config;

    try {
        pt::ptree tree;
        pt::read_json(filename, tree);

        // Server settings
        config.port = tree.get<uint16_t>("server.port", config.port);
        config.tickRate = tree.get<uint32_t>("server.tick_rate", config.tickRate);

        // Network settings
        config.maxConnections = tree.get<uint32_t>("network.max_connections", config.maxConnections);
        config.timeoutMs = tree.get<uint32_t>("network.timeout_ms", config.timeoutMs);

        // Game settings
        config.startingLevel = tree.get<int32_t>("game.starting_level", config.startingLevel);
        config.startingSkillPoints = tree.get<int32_t>("game.starting_skill_points", config.startingSkillPoints);
        config.expMultiplier = tree.get<float>("game.exp_multiplier", config.expMultiplier);

        std::cout << "Loaded config from " << filename << std::endl;
    } catch (const pt::json_parser_error& e) {
        std::cerr << "Config parse error: " << e.what() << std::endl;
        std::cout << "Using default configuration" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Config load error: " << e.what() << std::endl;
        std::cout << "Using default configuration" << std::endl;
    }

    return config;
}

GameServer::GameServer(Config config)
    : config_(config)
    , tickInterval_(1000 / config.tickRate) {
}

GameServer::~GameServer() {
    shutdown();
}

bool GameServer::initialize() {
    // Create systems
    eventBus_ = std::make_shared<EventBus>();
    actorManager_ = std::make_unique<ActorManager>();
    actorManager_->setEventBus(eventBus_);
    combatSystem_ = std::make_unique<CombatSystem>(*actorManager_, *eventBus_);

    // Setup skill system
    SkillDatabase::instance().loadDefaultSkills();
    setupSkillTree();

    // Subscribe to events
    damageEventId_ = eventBus_->subscribe([this](const GameEvent& event) {
        if (auto* dmg = std::get_if<DamageEvent>(&event)) {
            onDamageEvent(*dmg);
        }
    });

    deathEventId_ = eventBus_->subscribe([this](const GameEvent& event) {
        if (auto* death = std::get_if<DeathEvent>(&event)) {
            onDeathEvent(*death);
        }
    });

    // Create TCP server
    server_ = std::make_unique<net::TcpServer>(config_.port);

    server_->onPacket([this](net::ConnectionPtr conn, const proto::Packet& packet) {
        handlePacket(conn, packet);
    });

    server_->onConnect([this](net::ConnectionPtr conn) {
        onConnect(conn);
    });

    server_->onDisconnect([this](net::ConnectionPtr conn) {
        onDisconnect(conn);
    });

    if (!server_->start()) {
        std::cerr << "Failed to start server" << std::endl;
        return false;
    }

    // Create tick timer using server's io_context
    tickTimer_ = std::make_unique<asio::steady_timer>(server_->getIoContext());

    running_ = true;
    std::cout << "Game server initialized on port " << config_.port << std::endl;
    return true;
}

void GameServer::run() {
    if (!running_) return;

    std::cout << "Game loop started (tick rate: " << config_.tickRate << " Hz)" << std::endl;

    // Start the tick timer
    scheduleNextTick();

    // Run the io_context - this blocks until stop() is called
    while (running_) {
        // Poll for network events and timers
        server_->getIoContext().poll();

        // Also poll the server for client data
        server_->poll(1);

        // Small sleep to prevent busy loop
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameServer::scheduleNextTick() {
    if (!running_ || !tickTimer_) return;

    tickTimer_->expires_after(tickInterval_);
    tickTimer_->async_wait([this](const boost::system::error_code& ec) {
        onTickTimer(ec);
    });
}

void GameServer::onTickTimer(const boost::system::error_code& ec) {
    if (ec || !running_) return;

    // Perform game tick
    tick();

    // Schedule next tick
    scheduleNextTick();
}

void GameServer::shutdown() {
    if (!running_) return;

    running_ = false;

    // Cancel the tick timer
    if (tickTimer_) {
        boost::system::error_code ec;
        tickTimer_->cancel(ec);
    }

    if (eventBus_) {
        eventBus_->unsubscribe(damageEventId_);
        eventBus_->unsubscribe(deathEventId_);
    }

    if (server_) {
        server_->stop();
    }

    connToCharacter_.clear();
    actorManager_->clear();

    std::cout << "Game server shutdown complete" << std::endl;
}

void GameServer::tick() {
    currentTick_++;
    actorManager_->updateAll(currentTick_);
}

void GameServer::setupSkillTree() {
    // Tier 1 skills (no prerequisites)
    skillTree_.addNode({1, {}, {4}, 1});      // Slash -> Power Strike
    skillTree_.addNode({2, {}, {5}, 1});      // Fireball -> Flame Wave
    skillTree_.addNode({3, {}, {6}, 1});      // Heal -> Regeneration

    // Tier 2 skills
    skillTree_.addNode({4, {1}, {7}, 2});     // Power Strike -> Berserk
    skillTree_.addNode({5, {2}, {8}, 2});     // Flame Wave -> Meteor
    skillTree_.addNode({6, {3}, {9}, 2});     // Regeneration -> Divine Shield

    // Tier 3 skills
    skillTree_.addNode({7, {4}, {}, 3});      // Berserk
    skillTree_.addNode({8, {5}, {}, 3});      // Meteor
    skillTree_.addNode({9, {6}, {}, 3});      // Divine Shield
}

void GameServer::handlePacket(net::ConnectionPtr conn, const proto::Packet& packet) {
    auto type = static_cast<proto::MessageType>(packet.type());

    switch (type) {
        case proto::MSG_LOGIN_REQUEST: {
            proto::LoginRequest req;
            if (req.ParseFromString(packet.payload())) {
                handleLogin(conn, req);
            }
            break;
        }
        case proto::MSG_LOGOUT:
            handleLogout(conn);
            break;
        case proto::MSG_ATTACK_REQUEST: {
            proto::AttackRequest req;
            if (req.ParseFromString(packet.payload())) {
                handleAttack(conn, req);
            }
            break;
        }
        case proto::MSG_SKILL_REQUEST: {
            proto::SkillRequest req;
            if (req.ParseFromString(packet.payload())) {
                handleSkillRequest(conn, req);
            }
            break;
        }
        case proto::MSG_LEARN_SKILL: {
            proto::LearnSkill req;
            if (req.ParseFromString(packet.payload())) {
                handleLearnSkill(conn, req);
            }
            break;
        }
        case proto::MSG_UPGRADE_SKILL: {
            proto::UpgradeSkill req;
            if (req.ParseFromString(packet.payload())) {
                handleUpgradeSkill(conn, req);
            }
            break;
        }
        case proto::MSG_CHAT: {
            proto::Chat chat;
            if (chat.ParseFromString(packet.payload())) {
                handleChat(conn, chat);
            }
            break;
        }
        case proto::MSG_PING: {
            proto::Ping ping;
            if (ping.ParseFromString(packet.payload())) {
                handlePing(conn, ping);
            }
            break;
        }
        default:
            std::cerr << "Unknown packet type: " << packet.type() << std::endl;
            break;
    }
}

void GameServer::handleLogin(net::ConnectionPtr conn, const proto::LoginRequest& req) {
    std::cout << "Login request from " << req.username() << std::endl;

    // Create character for this connection
    auto character = actorManager_->createActor<Character>(req.username());
    character->setSkillTree(skillTree_);

    // Random stats for variety
    character->setPrimaryStat("strength", 10 + (conn->getId() % 10));
    character->setPrimaryStat("intelligence", 10 + ((conn->getId() * 3) % 10));
    character->setPrimaryStat("agility", 10 + ((conn->getId() * 7) % 10));

    conn->setActorId(character->getId());
    connToCharacter_[conn->getId()] = character;

    // Send login response
    proto::LoginResponse response;
    response.set_success(true);
    response.set_actor_id(character->getId());
    response.set_message("Welcome to the game, " + req.username() + "!");
    *response.mutable_actor() = buildActorInfo(*character);

    conn->sendPacket(proto::MSG_LOGIN_RESPONSE, response);

    // Broadcast spawn to all other players
    proto::ActorSpawn spawn;
    *spawn.mutable_actor() = buildActorInfo(*character);
    server_->broadcastExcept(conn->getId(), proto::MSG_ACTOR_SPAWN, spawn);

    // Send existing actors to new player
    proto::ActorList actorList;
    for (auto& [connId, otherChar] : connToCharacter_) {
        if (connId != conn->getId()) {
            *actorList.add_actors() = buildActorInfo(*otherChar);
        }
    }
    if (actorList.actors_size() > 0) {
        conn->sendPacket(proto::MSG_ACTOR_LIST, actorList);
    }

    // Send skill list
    conn->sendPacket(proto::MSG_SKILL_LIST, buildSkillList(*character));

    std::cout << "Player " << req.username() << " joined (Actor ID: "
              << character->getId() << ")" << std::endl;
}

void GameServer::handleLogout(net::ConnectionPtr conn) {
    onDisconnect(conn);
    server_->disconnect(conn->getId());
}

void GameServer::handleAttack(net::ConnectionPtr conn, const proto::AttackRequest& req) {
    auto it = connToCharacter_.find(conn->getId());
    if (it == connToCharacter_.end()) return;

    auto& attacker = it->second;
    auto target = actorManager_->getActor(req.target_id());
    if (!target) return;

    // Process attack through combat system
    auto result = combatSystem_->handleBasicAttack(
        BasicAttack{attacker->getId(), req.target_id(), true}
    );

    // Send result to attacker
    proto::AttackResult attackResult;
    attackResult.set_attacker_id(attacker->getId());
    attackResult.set_target_id(req.target_id());
    attackResult.set_damage(result.finalDamage);
    attackResult.set_is_critical(result.isCritical);
    attackResult.set_is_dodged(result.isDodged);
    attackResult.set_target_hp(target->getRuntimeStats().currentHp);

    // Broadcast to all
    server_->broadcast(proto::MSG_ATTACK_RESULT, attackResult);
}

void GameServer::handleSkillRequest(net::ConnectionPtr conn, const proto::SkillRequest& req) {
    auto it = connToCharacter_.find(conn->getId());
    if (it == connToCharacter_.end()) return;

    auto& caster = it->second;

    proto::SkillResult result;
    result.set_caster_id(caster->getId());
    result.set_skill_id(req.skill_id());
    result.set_target_id(req.target_id());

    // Try to use the skill
    if (caster->useSkill(req.skill_id())) {
        result.set_success(true);

        // Get skill info
        auto* skill = SkillDatabase::instance().getSkill(req.skill_id());
        if (skill) {
            result.set_damage(caster->getSkillLevel(req.skill_id()) * 20);  // Simplified
            result.set_message(caster->getName() + " uses " + skill->getName() + "!");
        }

        // Apply damage if target exists
        if (req.target_id() != 0) {
            auto target = actorManager_->getActor(req.target_id());
            if (target) {
                target->takeDamage(result.damage());
            }
        }
    } else {
        result.set_success(false);
        result.set_message("Cannot use skill!");
    }

    server_->broadcast(proto::MSG_SKILL_RESULT, result);

    // Send updated skill list (cooldowns changed)
    conn->sendPacket(proto::MSG_SKILL_LIST, buildSkillList(*caster));
}

void GameServer::handleLearnSkill(net::ConnectionPtr conn, const proto::LearnSkill& req) {
    auto it = connToCharacter_.find(conn->getId());
    if (it == connToCharacter_.end()) return;

    auto& character = it->second;
    bool success = character->learnSkill(req.skill_id());

    // Send updated skill list
    conn->sendPacket(proto::MSG_SKILL_LIST, buildSkillList(*character));

    if (!success) {
        proto::Error error;
        error.set_code(1);
        error.set_message("Cannot learn this skill!");
        conn->sendPacket(proto::MSG_ERROR, error);
    }
}

void GameServer::handleUpgradeSkill(net::ConnectionPtr conn, const proto::UpgradeSkill& req) {
    auto it = connToCharacter_.find(conn->getId());
    if (it == connToCharacter_.end()) return;

    auto& character = it->second;
    bool success = character->upgradeSkill(req.skill_id());

    // Send updated skill list
    conn->sendPacket(proto::MSG_SKILL_LIST, buildSkillList(*character));

    if (!success) {
        proto::Error error;
        error.set_code(2);
        error.set_message("Cannot upgrade this skill!");
        conn->sendPacket(proto::MSG_ERROR, error);
    }
}

void GameServer::handleChat(net::ConnectionPtr conn, const proto::Chat& chat) {
    auto it = connToCharacter_.find(conn->getId());
    if (it == connToCharacter_.end()) return;

    proto::Chat broadcastChat;
    broadcastChat.set_sender_id(it->second->getId());
    broadcastChat.set_sender_name(it->second->getName());
    broadcastChat.set_message(chat.message());

    server_->broadcast(proto::MSG_CHAT, broadcastChat);
}

void GameServer::handlePing(net::ConnectionPtr conn, const proto::Ping& ping) {
    proto::Pong pong;
    pong.set_timestamp(ping.timestamp());
    conn->sendPacket(proto::MSG_PONG, pong);
}

void GameServer::onConnect(net::ConnectionPtr conn) {
    std::cout << "Connection #" << conn->getId() << " established" << std::endl;
}

void GameServer::onDisconnect(net::ConnectionPtr conn) {
    auto it = connToCharacter_.find(conn->getId());
    if (it != connToCharacter_.end()) {
        auto& character = it->second;

        // Broadcast despawn
        proto::ActorDespawn despawn;
        despawn.set_actor_id(character->getId());
        server_->broadcastExcept(conn->getId(), proto::MSG_ACTOR_DESPAWN, despawn);

        // Remove from manager
        actorManager_->removeActor(character->getId());
        connToCharacter_.erase(it);

        std::cout << "Player " << character->getName() << " left" << std::endl;
    }
}

void GameServer::onDamageEvent(const DamageEvent& event) {
    // Already handled in handleAttack
}

void GameServer::onDeathEvent(const DeathEvent& event) {
    // Broadcast death notification
    proto::Chat deathMsg;
    deathMsg.set_sender_id(0);
    deathMsg.set_sender_name("System");

    auto victim = actorManager_->getActor(event.actor);
    auto killer = actorManager_->getActor(event.killer);

    if (victim && killer) {
        deathMsg.set_message(victim->getName() + " was killed by " + killer->getName() + "!");
    } else if (victim) {
        deathMsg.set_message(victim->getName() + " has died!");
    }

    server_->broadcast(proto::MSG_CHAT, deathMsg);
}

proto::ActorInfo GameServer::buildActorInfo(const Character& character) {
    proto::ActorInfo info;
    info.set_id(character.getId());
    info.set_name(character.getName());
    info.set_level(character.getLevel());
    info.set_current_hp(character.getRuntimeStats().currentHp);
    info.set_max_hp(character.getDerivedStats().maxHp);
    info.set_current_mp(character.getRuntimeStats().currentMp);
    info.set_max_mp(character.getDerivedStats().maxMp);

    auto* stats = info.mutable_stats();
    const auto& primary = character.getPrimaryStats();
    stats->set_strength(primary.strength);
    stats->set_agility(primary.agility);
    stats->set_intelligence(primary.intelligence);
    stats->set_vitality(primary.vitality);
    stats->set_wisdom(primary.wisdom);
    stats->set_luck(primary.luck);

    return info;
}

proto::SkillList GameServer::buildSkillList(const Character& character) {
    proto::SkillList list;
    list.set_skill_points(character.getSkillPoints());

    for (SkillId id : character.getLearnedSkills()) {
        auto* skill = SkillDatabase::instance().getSkill(id);
        if (skill) {
            auto* info = list.add_skills();
            info->set_id(id);
            info->set_name(skill->getName());
            info->set_level(character.getSkillLevel(id));
            info->set_max_level(skill->getMaxLevel());
            info->set_mana_cost(skill->getManaCost());
            info->set_cooldown(skill->getCooldown());
        }
    }

    return list;
}

} // namespace mmorpg
