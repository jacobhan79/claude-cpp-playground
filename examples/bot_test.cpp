#include "client/TestBot.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <random>
#include <iomanip>

using namespace mmorpg::client;

// Helper function to poll all bots
void pollAllBots(std::vector<std::unique_ptr<TestBot>>& bots, int times = 10, int intervalMs = 50) {
    for (int i = 0; i < times; i++) {
        for (auto& bot : bots) {
            if (bot->isConnected()) {
                bot->poll(intervalMs);
            }
        }
    }
}

// Helper function to print combat stats
void printCombatStats(const TestBot& bot) {
    const auto& stats = bot.getCombatStats();
    std::cout << "[" << bot.getName() << "] Combat Stats:" << std::endl;
    std::cout << "  Damage Dealt: " << stats.totalDamageDealt << std::endl;
    std::cout << "  Damage Received: " << stats.totalDamageReceived << std::endl;
    std::cout << "  Attacks Landed: " << stats.attacksLanded << std::endl;
    std::cout << "  Critical Hits: " << stats.criticalHits << std::endl;
    std::cout << "  Skills Used: " << stats.skillsUsed << std::endl;
    std::cout << "  Heals: " << stats.healsPerformed << " (Total: " << stats.totalHealing << ")" << std::endl;
}

void runSingleBotTest(const std::string& host, uint16_t port) {
    std::cout << "\n=== Single Bot Test ===" << std::endl;

    TestBot bot("TestPlayer1");

    if (!bot.connect(host, port)) {
        std::cerr << "Failed to connect!" << std::endl;
        return;
    }

    // Login
    bot.login();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bot.poll(500);

    // Learn skills
    std::cout << "\n--- Learning Skills ---" << std::endl;
    bot.learnSkill(1);  // Slash
    bot.poll(200);
    bot.learnSkill(2);  // Fireball
    bot.poll(200);
    bot.learnSkill(3);  // Heal
    bot.poll(200);

    // Ping test
    std::cout << "\n--- Ping Test ---" << std::endl;
    bot.ping();
    bot.poll(500);

    // Chat
    std::cout << "\n--- Chat Test ---" << std::endl;
    bot.chat("Hello from TestBot!");
    bot.poll(200);

    std::cout << "\n--- Single Bot Test Complete ---" << std::endl;
    bot.disconnect();
}

void runMultiBotTest(const std::string& host, uint16_t port, int numBots) {
    std::cout << "\n=== Multi Bot Test (" << numBots << " bots) ===" << std::endl;

    std::vector<std::unique_ptr<TestBot>> bots;

    // Create and connect bots
    for (int i = 0; i < numBots; i++) {
        auto bot = std::make_unique<TestBot>("Bot" + std::to_string(i + 1));
        if (bot->connect(host, port)) {
            bot->login();
            bots.push_back(std::move(bot));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Poll all bots to receive login response
    std::cout << "\n--- Waiting for login responses ---" << std::endl;
    for (int i = 0; i < 10; i++) {
        for (auto& bot : bots) {
            bot->poll(50);
        }
    }

    // Each bot learns skills
    std::cout << "\n--- Bots learning skills ---" << std::endl;
    for (auto& bot : bots) {
        bot->learnSkill(1);
        bot->learnSkill(2);
    }
    for (auto& bot : bots) {
        bot->poll(100);
    }

    // Combat test - bots attack each other
    if (bots.size() >= 2) {
        std::cout << "\n--- Combat Test ---" << std::endl;

        // Bot 0 attacks Bot 1
        uint32_t target = bots[1]->getActorId();
        std::cout << "Bot1 (ID:" << bots[0]->getActorId()
                  << ") attacks Bot2 (ID:" << target << ")" << std::endl;

        for (int i = 0; i < 5; i++) {
            bots[0]->attack(target);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            for (auto& bot : bots) {
                bot->poll(50);
            }
        }

        // Bot 1 uses skill on Bot 0
        std::cout << "\nBot2 uses Fireball on Bot1" << std::endl;
        bots[1]->useSkill(2, bots[0]->getActorId());
        for (auto& bot : bots) {
            bot->poll(100);
        }
    }

    // Chat test
    std::cout << "\n--- Chat Test ---" << std::endl;
    bots[0]->chat("Hello everyone!");
    for (auto& bot : bots) {
        bot->poll(100);
    }

    // Disconnect one bot
    if (bots.size() > 1) {
        std::cout << "\n--- Disconnect Test ---" << std::endl;
        bots.back()->disconnect();
        bots.pop_back();

        // Others should receive despawn
        for (auto& bot : bots) {
            bot->poll(200);
        }
    }

    std::cout << "\n--- Multi Bot Test Complete ---" << std::endl;

    // Cleanup
    for (auto& bot : bots) {
        bot->disconnect();
    }
}

void runStressTest(const std::string& host, uint16_t port, int numBots, int duration) {
    std::cout << "\n=== Stress Test (" << numBots << " bots, " << duration << "s) ===" << std::endl;

    std::vector<std::unique_ptr<TestBot>> bots;

    // Create and connect bots
    for (int i = 0; i < numBots; i++) {
        auto bot = std::make_unique<TestBot>("StressBot" + std::to_string(i + 1));
        if (bot->connect(host, port)) {
            bot->login();
            bots.push_back(std::move(bot));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Connected " << bots.size() << " bots" << std::endl;

    // Wait for logins
    for (int i = 0; i < 20; i++) {
        for (auto& bot : bots) {
            bot->poll(10);
        }
    }

    // Run for duration seconds
    auto startTime = std::chrono::steady_clock::now();
    int actionCount = 0;

    while (std::chrono::steady_clock::now() - startTime < std::chrono::seconds(duration)) {
        for (size_t i = 0; i < bots.size(); i++) {
            auto& bot = bots[i];

            // Random action
            int action = actionCount % 4;
            if (action == 0 && bots.size() > 1) {
                // Attack random other bot
                size_t targetIdx = (i + 1) % bots.size();
                bot->attack(bots[targetIdx]->getActorId());
            } else if (action == 1) {
                bot->ping();
            } else if (action == 2) {
                bot->chat("Msg " + std::to_string(actionCount));
            } else {
                bot->useSkill(1, bot->getActorId());
            }

            bot->poll(5);
            actionCount++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "Performed " << actionCount << " actions" << std::endl;

    // Cleanup
    for (auto& bot : bots) {
        bot->disconnect();
    }

    std::cout << "--- Stress Test Complete ---" << std::endl;
}

// ============================================================================
// Network Scenarios
// ============================================================================

void runReconnectTest(const std::string& host, uint16_t port) {
    std::cout << "\n=== Reconnect Test ===" << std::endl;

    TestBot bot("ReconnectBot");
    int successCount = 0;

    // Test 1: Normal reconnect
    std::cout << "\n--- Test 1: Normal Reconnect ---" << std::endl;
    if (!bot.connect(host, port)) {
        std::cerr << "Initial connection failed!" << std::endl;
        return;
    }
    bot.login();
    bot.poll(500);

    uint32_t firstActorId = bot.getActorId();
    std::cout << "First login Actor ID: " << firstActorId << std::endl;

    bot.learnSkill(1);  // Learn a skill to verify state
    bot.poll(200);

    bot.disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (bot.reconnect()) {
        bot.login();
        bot.poll(500);
        std::cout << "Reconnected! New Actor ID: " << bot.getActorId() << std::endl;
        successCount++;
    } else {
        std::cout << "Reconnect failed!" << std::endl;
    }

    // Test 2: Multiple rapid reconnects
    std::cout << "\n--- Test 2: Multiple Rapid Reconnects (5x) ---" << std::endl;
    bool allReconnectsSuccess = true;
    for (int i = 0; i < 5; i++) {
        bot.disconnect();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (!bot.reconnect()) {
            std::cout << "Reconnect " << (i + 1) << " failed!" << std::endl;
            allReconnectsSuccess = false;
            break;
        }
        bot.login();
        bot.poll(200);
        std::cout << "Reconnect " << (i + 1) << " successful" << std::endl;
    }
    if (allReconnectsSuccess) successCount++;

    // Test 3: Reconnect with delay
    std::cout << "\n--- Test 3: Reconnect With Delay ---" << std::endl;
    bot.disconnect();
    if (bot.reconnectWithDelay(1000)) {
        bot.login();
        bot.poll(500);
        std::cout << "Delayed reconnect successful!" << std::endl;
        successCount++;
    }

    std::cout << "\n=== Reconnect Test Results: " << successCount << "/3 passed ===" << std::endl;
    bot.disconnect();
}

void runTimeoutTest(const std::string& host, uint16_t port) {
    std::cout << "\n=== Timeout Test ===" << std::endl;

    TestBot bot("TimeoutBot");

    if (!bot.connect(host, port)) {
        std::cerr << "Connection failed!" << std::endl;
        return;
    }
    bot.login();
    bot.poll(500);

    // Test: Keep connection alive with periodic pings
    std::cout << "\n--- Keep-Alive Test (15 seconds) ---" << std::endl;
    auto start = std::chrono::steady_clock::now();
    int pingCount = 0;

    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(15)) {
        bot.ping();
        pingCount++;

        // Wait 3 seconds between pings
        for (int i = 0; i < 30 && bot.isConnected(); i++) {
            bot.poll(100);
        }

        if (!bot.isConnected()) {
            std::cout << "Connection lost after " << pingCount << " pings!" << std::endl;
            break;
        }
        std::cout << "Ping " << pingCount << " - Connection alive" << std::endl;
    }

    if (bot.isConnected()) {
        std::cout << "\n=== Timeout Test PASSED - Connection maintained ===" << std::endl;
    } else {
        std::cout << "\n=== Timeout Test FAILED - Connection lost ===" << std::endl;
    }

    bot.disconnect();
}

void runConcurrentTest(const std::string& host, uint16_t port, int numBots) {
    std::cout << "\n=== Concurrent Connection Test (" << numBots << " bots) ===" << std::endl;

    std::vector<std::unique_ptr<TestBot>> bots;
    auto start = std::chrono::steady_clock::now();

    // Connect all bots as fast as possible
    std::cout << "\n--- Connecting bots ---" << std::endl;
    int connectedCount = 0;
    for (int i = 0; i < numBots; i++) {
        auto bot = std::make_unique<TestBot>("ConcurrentBot" + std::to_string(i + 1));
        if (bot->connect(host, port)) {
            bot->login();
            connectedCount++;
            bots.push_back(std::move(bot));
        }
    }

    auto connectTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    std::cout << "Connected " << connectedCount << "/" << numBots
              << " bots in " << connectTime << "ms" << std::endl;

    // Wait for all login responses
    pollAllBots(bots, 20, 50);

    // Count successful logins
    int loggedInCount = 0;
    for (auto& bot : bots) {
        if (bot->isConnected() && bot->getActorId() != 0) {
            loggedInCount++;
        }
    }
    std::cout << "Logged in: " << loggedInCount << "/" << connectedCount << std::endl;

    // Test: All bots send messages simultaneously
    std::cout << "\n--- Simultaneous Chat Test ---" << std::endl;
    for (auto& bot : bots) {
        if (bot->isConnected()) {
            bot->chat("Hello from " + bot->getName());
        }
    }
    pollAllBots(bots, 10, 50);

    // Test: All bots learn skills
    std::cout << "\n--- Simultaneous Skill Learn ---" << std::endl;
    for (auto& bot : bots) {
        if (bot->isConnected()) {
            bot->learnSkill(1);
            bot->learnSkill(2);
        }
    }
    pollAllBots(bots, 10, 50);

    // Report
    std::cout << "\n=== Concurrent Test Results ===" << std::endl;
    std::cout << "  Connection Time: " << connectTime << "ms" << std::endl;
    std::cout << "  Connected: " << connectedCount << "/" << numBots << std::endl;
    std::cout << "  Logged In: " << loggedInCount << "/" << connectedCount << std::endl;
    std::cout << "  Success Rate: " << (loggedInCount * 100 / numBots) << "%" << std::endl;

    // Cleanup
    for (auto& bot : bots) {
        bot->disconnect();
    }
}

void runPacketOrderTest(const std::string& host, uint16_t port) {
    std::cout << "\n=== Packet Order Test ===" << std::endl;

    TestBot bot("PacketOrderBot");

    if (!bot.connect(host, port)) {
        std::cerr << "Connection failed!" << std::endl;
        return;
    }
    bot.login();
    bot.poll(500);

    // Test 1: Sequential packets
    std::cout << "\n--- Test 1: Sequential Packets ---" << std::endl;
    bot.clearReceivedPackets();
    for (int i = 0; i < 10; i++) {
        bot.chat("Message " + std::to_string(i + 1));
        bot.poll(50);
    }
    bot.poll(500);
    std::cout << "Sent 10 sequential messages, received "
              << bot.getReceivedPackets().size() << " packets" << std::endl;

    // Test 2: Burst packets (no delay between sends)
    std::cout << "\n--- Test 2: Burst Packets ---" << std::endl;
    bot.clearReceivedPackets();
    for (int i = 0; i < 20; i++) {
        bot.ping();
    }
    // Wait for all responses
    bot.waitForPackets(20, 3000);
    std::cout << "Sent 20 burst pings, received "
              << bot.getReceivedPackets().size() << " packets" << std::endl;

    // Test 3: Mixed packet types
    std::cout << "\n--- Test 3: Mixed Packet Types ---" << std::endl;
    bot.clearReceivedPackets();
    bot.learnSkill(1);
    bot.chat("Learning skill");
    bot.ping();
    bot.learnSkill(2);
    bot.chat("Another message");
    bot.ping();
    bot.waitForPackets(6, 2000);
    std::cout << "Sent 6 mixed packets, received "
              << bot.getReceivedPackets().size() << " packets" << std::endl;

    std::cout << "\n=== Packet Order Test Complete ===" << std::endl;
    bot.disconnect();
}

// ============================================================================
// Combat Scenarios
// ============================================================================

void runSkillComboTest(const std::string& host, uint16_t port) {
    std::cout << "\n=== Skill Combo Test ===" << std::endl;

    TestBot bot("ComboMaster");

    if (!bot.connect(host, port)) {
        std::cerr << "Connection failed!" << std::endl;
        return;
    }
    bot.login();
    bot.poll(500);

    // Learn all Tier 1 skills
    std::cout << "\n--- Learning Skills ---" << std::endl;
    bot.learnSkill(1);  // Slash
    bot.learnSkill(2);  // Fireball
    bot.learnSkill(3);  // Heal
    bot.poll(500);

    // Combo 1: Basic Attack Combo (Slash x3)
    std::cout << "\n--- Combo 1: Triple Slash ---" << std::endl;
    bot.resetCombatStats();
    for (int i = 0; i < 3; i++) {
        bot.useSkill(1, bot.getActorId());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bot.poll(100);
    }

    // Combo 2: Fire Burst (Fireball x3)
    std::cout << "\n--- Combo 2: Fire Burst ---" << std::endl;
    for (int i = 0; i < 3; i++) {
        bot.useSkill(2, bot.getActorId());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bot.poll(100);
    }

    // Combo 3: Heal Recovery
    std::cout << "\n--- Combo 3: Heal Recovery ---" << std::endl;
    for (int i = 0; i < 3; i++) {
        bot.useSkill(3, bot.getActorId());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bot.poll(100);
    }

    // Combo 4: Mixed Combo (Slash -> Fireball -> Heal)
    std::cout << "\n--- Combo 4: Mixed Combo ---" << std::endl;
    bot.useSkill(1, bot.getActorId());
    bot.poll(100);
    bot.useSkill(2, bot.getActorId());
    bot.poll(100);
    bot.useSkill(3, bot.getActorId());
    bot.poll(100);

    // Print results
    std::cout << "\n--- Combo Results ---" << std::endl;
    printCombatStats(bot);

    std::cout << "\n=== Skill Combo Test Complete ===" << std::endl;
    bot.disconnect();
}

void runBuffDebuffTest(const std::string& host, uint16_t port) {
    std::cout << "\n=== Buff/Debuff Test ===" << std::endl;

    std::vector<std::unique_ptr<TestBot>> bots;
    auto buffer = std::make_unique<TestBot>("Buffer");
    auto target = std::make_unique<TestBot>("Target");

    // Connect both bots
    if (!buffer->connect(host, port) || !target->connect(host, port)) {
        std::cerr << "Connection failed!" << std::endl;
        return;
    }

    buffer->login();
    target->login();
    bots.push_back(std::move(buffer));
    bots.push_back(std::move(target));
    pollAllBots(bots, 10, 50);

    // Learn skills
    bots[0]->learnSkill(1);  // Slash
    bots[0]->learnSkill(2);  // Fireball
    bots[0]->learnSkill(3);  // Heal
    bots[1]->learnSkill(1);
    bots[1]->learnSkill(3);
    pollAllBots(bots, 5, 100);

    uint32_t targetId = bots[1]->getActorId();

    // Test 1: Buff with Heal
    std::cout << "\n--- Test 1: Heal Buff ---" << std::endl;
    bots[0]->useSkill(3, targetId);  // Heal target
    pollAllBots(bots, 5, 100);

    // Test 2: Damage with Fireball
    std::cout << "\n--- Test 2: Damage with Fireball ---" << std::endl;
    bots[0]->useSkill(2, targetId);
    pollAllBots(bots, 5, 100);

    // Test 3: Self-buff
    std::cout << "\n--- Test 3: Self Heal ---" << std::endl;
    bots[1]->useSkill(3, bots[1]->getActorId());
    pollAllBots(bots, 5, 100);

    // Test 4: Multiple buffs on same target
    std::cout << "\n--- Test 4: Multiple Skills on Target ---" << std::endl;
    for (int i = 0; i < 3; i++) {
        bots[0]->useSkill(1, targetId);
        bots[0]->useSkill(2, targetId);
        bots[1]->useSkill(3, bots[1]->getActorId());
        pollAllBots(bots, 3, 50);
    }

    // Print stats
    std::cout << "\n--- Results ---" << std::endl;
    printCombatStats(*bots[0]);
    printCombatStats(*bots[1]);

    std::cout << "\n=== Buff/Debuff Test Complete ===" << std::endl;
    for (auto& bot : bots) {
        bot->disconnect();
    }
}

void runCombatSituationsTest(const std::string& host, uint16_t port, int numBots) {
    std::cout << "\n=== Combat Situations Test (" << numBots << " bots) ===" << std::endl;

    std::vector<std::unique_ptr<TestBot>> bots;

    // Create and connect bots
    for (int i = 0; i < numBots; i++) {
        auto bot = std::make_unique<TestBot>("Fighter" + std::to_string(i + 1));
        if (bot->connect(host, port)) {
            bot->login();
            bots.push_back(std::move(bot));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    pollAllBots(bots, 10, 50);

    // All bots learn skills
    for (auto& bot : bots) {
        bot->learnSkill(1);
        bot->learnSkill(2);
        bot->learnSkill(3);
    }
    pollAllBots(bots, 5, 100);

    // Situation 1: 1v1 Duel
    if (bots.size() >= 2) {
        std::cout << "\n--- Situation 1: 1v1 Duel ---" << std::endl;
        std::cout << bots[0]->getName() << " vs " << bots[1]->getName() << std::endl;

        for (int round = 0; round < 5; round++) {
            bots[0]->attack(bots[1]->getActorId());
            bots[1]->attack(bots[0]->getActorId());
            pollAllBots(bots, 3, 50);
        }
    }

    // Situation 2: 2v2 Team Battle
    if (bots.size() >= 4) {
        std::cout << "\n--- Situation 2: 2v2 Team Battle ---" << std::endl;
        std::cout << "Team A: " << bots[0]->getName() << ", " << bots[1]->getName() << std::endl;
        std::cout << "Team B: " << bots[2]->getName() << ", " << bots[3]->getName() << std::endl;

        for (int round = 0; round < 5; round++) {
            // Team A attacks Team B
            bots[0]->attack(bots[2]->getActorId());
            bots[1]->attack(bots[3]->getActorId());
            // Team B attacks Team A
            bots[2]->attack(bots[0]->getActorId());
            bots[3]->attack(bots[1]->getActorId());
            pollAllBots(bots, 3, 50);
        }
    }

    // Situation 3: Free For All
    std::cout << "\n--- Situation 3: Free For All ---" << std::endl;
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int round = 0; round < 10; round++) {
        for (size_t i = 0; i < bots.size(); i++) {
            // Attack random other bot
            std::uniform_int_distribution<> dis(0, bots.size() - 1);
            size_t targetIdx = dis(gen);
            while (targetIdx == i && bots.size() > 1) {
                targetIdx = dis(gen);
            }
            bots[i]->attack(bots[targetIdx]->getActorId());
        }
        pollAllBots(bots, 2, 50);
    }

    // Situation 4: Focus Fire
    if (bots.size() >= 3) {
        std::cout << "\n--- Situation 4: Focus Fire ---" << std::endl;
        std::cout << "Everyone attacks " << bots[0]->getName() << std::endl;

        for (int round = 0; round < 5; round++) {
            for (size_t i = 1; i < bots.size(); i++) {
                bots[i]->attack(bots[0]->getActorId());
            }
            // Target tries to heal
            bots[0]->useSkill(3, bots[0]->getActorId());
            pollAllBots(bots, 3, 50);
        }
    }

    // Print all stats
    std::cout << "\n--- Final Combat Statistics ---" << std::endl;
    for (auto& bot : bots) {
        printCombatStats(*bot);
        std::cout << std::endl;
    }

    std::cout << "=== Combat Situations Test Complete ===" << std::endl;
    for (auto& bot : bots) {
        bot->disconnect();
    }
}

// ============================================================================
// Gameplay Scenarios
// ============================================================================

void runDungeonTest(const std::string& host, uint16_t port) {
    std::cout << "\n=== Dungeon Exploration Test ===" << std::endl;

    TestBot bot("DungeonExplorer");

    if (!bot.connect(host, port)) {
        std::cerr << "Connection failed!" << std::endl;
        return;
    }
    bot.login();
    bot.poll(500);

    // Prepare for dungeon
    std::cout << "\n--- Preparing for Dungeon ---" << std::endl;
    bot.learnSkill(1);  // Slash - attack
    bot.learnSkill(2);  // Fireball - AoE
    bot.learnSkill(3);  // Heal - survival
    bot.poll(500);
    bot.chat("Entering the dungeon...");
    bot.poll(100);

    // Explore 5 rooms
    for (int room = 1; room <= 5; room++) {
        std::cout << "\n--- Room " << room << " ---" << std::endl;
        bot.chat("Entering room " + std::to_string(room));
        bot.poll(100);

        // Combat in each room (simulate monster fights)
        int monstersInRoom = room;  // More monsters in later rooms
        for (int monster = 0; monster < monstersInRoom; monster++) {
            std::cout << "Fighting monster " << (monster + 1) << "/" << monstersInRoom << std::endl;

            // Attack pattern: Slash, Slash, Fireball
            bot.useSkill(1, bot.getActorId());
            bot.poll(100);
            bot.useSkill(1, bot.getActorId());
            bot.poll(100);
            bot.useSkill(2, bot.getActorId());
            bot.poll(100);
        }

        // Heal after room if HP might be low
        std::cout << "Healing after combat..." << std::endl;
        bot.useSkill(3, bot.getActorId());
        bot.poll(200);
    }

    // Boss room
    std::cout << "\n--- Boss Room ---" << std::endl;
    bot.chat("Boss fight begins!");
    bot.poll(100);

    for (int phase = 1; phase <= 3; phase++) {
        std::cout << "Boss Phase " << phase << std::endl;
        for (int i = 0; i < 3; i++) {
            bot.useSkill(1, bot.getActorId());
            bot.useSkill(2, bot.getActorId());
            bot.poll(100);
        }
        bot.useSkill(3, bot.getActorId());  // Heal between phases
        bot.poll(200);
    }

    bot.chat("Dungeon cleared!");
    bot.poll(100);

    // Final stats
    std::cout << "\n--- Dungeon Complete ---" << std::endl;
    printCombatStats(bot);

    std::cout << "\n=== Dungeon Exploration Test Complete ===" << std::endl;
    bot.disconnect();
}

void runPartyTest(const std::string& host, uint16_t port, int numMembers) {
    std::cout << "\n=== Party Play Test (" << numMembers << " members) ===" << std::endl;

    if (numMembers < 2) numMembers = 2;
    if (numMembers > 5) numMembers = 5;

    std::vector<std::unique_ptr<TestBot>> party;
    std::vector<std::string> roles = {"Tank", "DPS", "DPS", "Healer", "Support"};

    // Create party
    for (int i = 0; i < numMembers; i++) {
        std::string name = roles[i % roles.size()] + std::to_string(i + 1);
        auto bot = std::make_unique<TestBot>(name);
        if (bot->connect(host, port)) {
            bot->login();
            party.push_back(std::move(bot));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    pollAllBots(party, 10, 50);

    std::cout << "\n--- Party Formed ---" << std::endl;
    for (auto& member : party) {
        std::cout << "  " << member->getName() << " (ID: " << member->getActorId() << ")" << std::endl;
    }

    // Assign skills based on role
    std::cout << "\n--- Assigning Skills ---" << std::endl;
    for (size_t i = 0; i < party.size(); i++) {
        if (i == 0) {  // Tank
            party[i]->learnSkill(1);  // Slash
            std::cout << party[i]->getName() << " learned Slash (tank)" << std::endl;
        } else if (i == party.size() - 1) {  // Healer
            party[i]->learnSkill(3);  // Heal
            std::cout << party[i]->getName() << " learned Heal (healer)" << std::endl;
        } else {  // DPS
            party[i]->learnSkill(2);  // Fireball
            std::cout << party[i]->getName() << " learned Fireball (dps)" << std::endl;
        }
    }
    pollAllBots(party, 5, 100);

    // Party raid simulation
    std::cout << "\n--- Party Raid ---" << std::endl;
    uint32_t tankId = party[0]->getActorId();

    for (int wave = 1; wave <= 3; wave++) {
        std::cout << "\nWave " << wave << ":" << std::endl;

        // Tank engages
        std::cout << "  Tank engaging..." << std::endl;
        party[0]->useSkill(1, tankId);
        pollAllBots(party, 2, 50);

        // DPS attacks
        std::cout << "  DPS attacking..." << std::endl;
        for (size_t i = 1; i < party.size() - 1; i++) {
            party[i]->useSkill(2, tankId);
        }
        pollAllBots(party, 2, 50);

        // Healer heals tank
        if (party.size() > 1) {
            std::cout << "  Healer healing tank..." << std::endl;
            party.back()->useSkill(3, tankId);
        }
        pollAllBots(party, 3, 100);
    }

    // Victory celebration
    std::cout << "\n--- Victory! ---" << std::endl;
    for (auto& member : party) {
        member->chat("GG!");
    }
    pollAllBots(party, 5, 50);

    // Print party stats
    std::cout << "\n--- Party Statistics ---" << std::endl;
    for (auto& member : party) {
        printCombatStats(*member);
        std::cout << std::endl;
    }

    std::cout << "=== Party Play Test Complete ===" << std::endl;
    for (auto& member : party) {
        member->disconnect();
    }
}

void runQuestTest(const std::string& host, uint16_t port) {
    std::cout << "\n=== Quest Test ===" << std::endl;

    TestBot bot("QuestHero");

    if (!bot.connect(host, port)) {
        std::cerr << "Connection failed!" << std::endl;
        return;
    }
    bot.login();
    bot.poll(500);

    // Quest 1: Learn basic skills
    std::cout << "\n--- Quest 1: Skill Training ---" << std::endl;
    bot.chat("[Quest] Training begins!");
    bot.poll(100);

    bot.learnSkill(1);
    bot.poll(200);
    bot.chat("[Quest] Learned Slash!");

    bot.learnSkill(2);
    bot.poll(200);
    bot.chat("[Quest] Learned Fireball!");

    bot.learnSkill(3);
    bot.poll(200);
    bot.chat("[Quest] Learned Heal! Training complete!");
    std::cout << "Quest 1 Complete!" << std::endl;

    // Quest 2: Combat training (use each skill 3 times)
    std::cout << "\n--- Quest 2: Combat Practice ---" << std::endl;
    bot.chat("[Quest] Starting combat practice...");
    bot.poll(100);

    for (int skill = 1; skill <= 3; skill++) {
        for (int use = 0; use < 3; use++) {
            bot.useSkill(skill, bot.getActorId());
            bot.poll(100);
        }
    }
    bot.chat("[Quest] Combat practice complete!");
    std::cout << "Quest 2 Complete!" << std::endl;

    // Quest 3: Exploration (simulate moving through areas)
    std::cout << "\n--- Quest 3: Exploration ---" << std::endl;
    std::vector<std::string> areas = {"Forest", "Cave", "Mountain", "Castle"};

    for (const auto& area : areas) {
        bot.chat("[Quest] Exploring " + area + "...");
        bot.poll(100);

        // Fight in each area
        bot.useSkill(1, bot.getActorId());
        bot.useSkill(2, bot.getActorId());
        bot.poll(200);

        bot.chat("[Quest] " + area + " cleared!");
        bot.poll(100);
    }
    std::cout << "Quest 3 Complete!" << std::endl;

    // Quest 4: Skill upgrade
    std::cout << "\n--- Quest 4: Skill Mastery ---" << std::endl;
    bot.chat("[Quest] Upgrading skills...");
    bot.poll(100);

    bot.upgradeSkill(1);
    bot.poll(200);
    bot.upgradeSkill(2);
    bot.poll(200);
    bot.chat("[Quest] Skills upgraded!");
    std::cout << "Quest 4 Complete!" << std::endl;

    // Final report
    std::cout << "\n--- All Quests Complete! ---" << std::endl;
    bot.chat("All quests completed! I am now a hero!");
    bot.poll(100);
    printCombatStats(bot);

    std::cout << "\n=== Quest Test Complete ===" << std::endl;
    bot.disconnect();
}

void runBossRaidTest(const std::string& host, uint16_t port, int numRaiders) {
    std::cout << "\n=== Boss Raid Test (" << numRaiders << " raiders) ===" << std::endl;

    if (numRaiders < 2) numRaiders = 2;

    std::vector<std::unique_ptr<TestBot>> raiders;

    // Create raid group
    for (int i = 0; i < numRaiders; i++) {
        auto bot = std::make_unique<TestBot>("Raider" + std::to_string(i + 1));
        if (bot->connect(host, port)) {
            bot->login();
            raiders.push_back(std::move(bot));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    pollAllBots(raiders, 10, 50);

    // All raiders learn skills
    for (auto& raider : raiders) {
        raider->learnSkill(1);
        raider->learnSkill(2);
        raider->learnSkill(3);
    }
    pollAllBots(raiders, 5, 100);

    // Use first raider as "boss" target
    uint32_t bossTarget = raiders[0]->getActorId();

    // Phase 1: Initial Engage
    std::cout << "\n--- Phase 1: Initial Engage ---" << std::endl;
    raiders[0]->chat("BOSS PULL!");
    pollAllBots(raiders, 2, 50);

    for (auto& raider : raiders) {
        raider->useSkill(1, bossTarget);  // Everyone uses Slash
    }
    pollAllBots(raiders, 5, 100);

    // Phase 2: DPS Phase
    std::cout << "\n--- Phase 2: DPS Phase ---" << std::endl;
    raiders[0]->chat("DPS NOW!");
    pollAllBots(raiders, 2, 50);

    for (int wave = 0; wave < 3; wave++) {
        for (auto& raider : raiders) {
            raider->useSkill(2, bossTarget);  // Fireball spam
        }
        pollAllBots(raiders, 3, 100);
    }

    // Phase 3: Healing Check
    std::cout << "\n--- Phase 3: Healing Check ---" << std::endl;
    raiders[0]->chat("HEALERS!");
    pollAllBots(raiders, 2, 50);

    for (auto& raider : raiders) {
        raider->useSkill(3, raider->getActorId());  // Everyone self-heals
    }
    pollAllBots(raiders, 5, 100);

    // Phase 4: Burn Phase
    std::cout << "\n--- Phase 4: Burn Phase ---" << std::endl;
    raiders[0]->chat("BURN! BURN! BURN!");
    pollAllBots(raiders, 2, 50);

    for (int wave = 0; wave < 5; wave++) {
        for (auto& raider : raiders) {
            raider->useSkill(1, bossTarget);
            raider->useSkill(2, bossTarget);
        }
        pollAllBots(raiders, 2, 50);
    }

    // Victory
    std::cout << "\n--- BOSS DEFEATED! ---" << std::endl;
    for (auto& raider : raiders) {
        raider->chat("VICTORY!");
    }
    pollAllBots(raiders, 5, 50);

    // Raid statistics
    std::cout << "\n--- Raid Statistics ---" << std::endl;
    int totalDamage = 0;
    int totalHealing = 0;
    for (auto& raider : raiders) {
        totalDamage += raider->getCombatStats().totalDamageDealt;
        totalHealing += raider->getCombatStats().totalHealing;
    }
    std::cout << "Total Raid Damage: " << totalDamage << std::endl;
    std::cout << "Total Raid Healing: " << totalHealing << std::endl;
    std::cout << "\nIndividual Stats:" << std::endl;
    for (auto& raider : raiders) {
        std::cout << "  " << raider->getName() << ": "
                  << raider->getCombatStats().totalDamageDealt << " damage, "
                  << raider->getCombatStats().skillsUsed << " skills" << std::endl;
    }

    std::cout << "\n=== Boss Raid Test Complete ===" << std::endl;
    for (auto& raider : raiders) {
        raider->disconnect();
    }
}

// ============================================================================
// All Tests
// ============================================================================

void runAllTests(const std::string& host, uint16_t port) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "    Running All Test Scenarios" << std::endl;
    std::cout << "========================================" << std::endl;

    std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"single", [&]() { runSingleBotTest(host, port); }},
        {"multi", [&]() { runMultiBotTest(host, port, 3); }},
        {"reconnect", [&]() { runReconnectTest(host, port); }},
        {"timeout", [&]() { runTimeoutTest(host, port); }},
        {"concurrent", [&]() { runConcurrentTest(host, port, 10); }},
        {"packet", [&]() { runPacketOrderTest(host, port); }},
        {"combo", [&]() { runSkillComboTest(host, port); }},
        {"buff", [&]() { runBuffDebuffTest(host, port); }},
        {"combat", [&]() { runCombatSituationsTest(host, port, 4); }},
        {"dungeon", [&]() { runDungeonTest(host, port); }},
        {"party", [&]() { runPartyTest(host, port, 3); }},
        {"quest", [&]() { runQuestTest(host, port); }},
        {"boss", [&]() { runBossRaidTest(host, port, 5); }},
    };

    int passed = 0;
    int failed = 0;

    for (const auto& [name, testFn] : tests) {
        std::cout << "\n>>> Running test: " << name << std::endl;
        try {
            testFn();
            passed++;
            std::cout << "<<< Test " << name << ": PASSED" << std::endl;
        } catch (const std::exception& e) {
            failed++;
            std::cerr << "<<< Test " << name << ": FAILED - " << e.what() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "    Test Summary: " << passed << "/" << (passed + failed) << " passed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " <host> <port> [test_type] [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Test types:" << std::endl;
    std::cout << "  === Basic ===" << std::endl;
    std::cout << "  single                  - Single bot test (default)" << std::endl;
    std::cout << "  multi <num_bots>        - Multi bot test" << std::endl;
    std::cout << "  stress <bots> <sec>     - Stress test" << std::endl;
    std::cout << std::endl;
    std::cout << "  === Gameplay ===" << std::endl;
    std::cout << "  dungeon                 - Dungeon exploration scenario" << std::endl;
    std::cout << "  party <num_members>     - Party play scenario (2-5)" << std::endl;
    std::cout << "  quest                   - Quest completion scenario" << std::endl;
    std::cout << "  boss <num_raiders>      - Boss raid scenario" << std::endl;
    std::cout << std::endl;
    std::cout << "  === Combat ===" << std::endl;
    std::cout << "  combo                   - Skill combo test" << std::endl;
    std::cout << "  buff                    - Buff/Debuff test" << std::endl;
    std::cout << "  combat <num_bots>       - Various combat situations" << std::endl;
    std::cout << std::endl;
    std::cout << "  === Network ===" << std::endl;
    std::cout << "  reconnect               - Reconnection test" << std::endl;
    std::cout << "  timeout                 - Timeout handling test" << std::endl;
    std::cout << "  concurrent <num_bots>   - Concurrent connection test" << std::endl;
    std::cout << "  packet                  - Packet ordering test" << std::endl;
    std::cout << std::endl;
    std::cout << "  === Suite ===" << std::endl;
    std::cout << "  all                     - Run all test scenarios" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program << " localhost 7777" << std::endl;
    std::cout << "  " << program << " localhost 7777 dungeon" << std::endl;
    std::cout << "  " << program << " localhost 7777 party 4" << std::endl;
    std::cout << "  " << program << " localhost 7777 concurrent 50" << std::endl;
    std::cout << "  " << program << " localhost 7777 all" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    std::string host = argv[1];
    uint16_t port = static_cast<uint16_t>(std::atoi(argv[2]));
    std::string testType = (argc > 3) ? argv[3] : "single";

    std::cout << "=== MMORPG Test Bot ===" << std::endl;
    std::cout << "Connecting to " << host << ":" << port << std::endl;

    // Basic tests
    if (testType == "single") {
        runSingleBotTest(host, port);
    } else if (testType == "multi") {
        int numBots = (argc > 4) ? std::atoi(argv[4]) : 3;
        runMultiBotTest(host, port, numBots);
    } else if (testType == "stress") {
        int numBots = (argc > 4) ? std::atoi(argv[4]) : 10;
        int duration = (argc > 5) ? std::atoi(argv[5]) : 10;
        runStressTest(host, port, numBots, duration);
    }
    // Gameplay tests
    else if (testType == "dungeon") {
        runDungeonTest(host, port);
    } else if (testType == "party") {
        int numMembers = (argc > 4) ? std::atoi(argv[4]) : 3;
        runPartyTest(host, port, numMembers);
    } else if (testType == "quest") {
        runQuestTest(host, port);
    } else if (testType == "boss") {
        int numRaiders = (argc > 4) ? std::atoi(argv[4]) : 5;
        runBossRaidTest(host, port, numRaiders);
    }
    // Combat tests
    else if (testType == "combo") {
        runSkillComboTest(host, port);
    } else if (testType == "buff") {
        runBuffDebuffTest(host, port);
    } else if (testType == "combat") {
        int numBots = (argc > 4) ? std::atoi(argv[4]) : 4;
        runCombatSituationsTest(host, port, numBots);
    }
    // Network tests
    else if (testType == "reconnect") {
        runReconnectTest(host, port);
    } else if (testType == "timeout") {
        runTimeoutTest(host, port);
    } else if (testType == "concurrent") {
        int numBots = (argc > 4) ? std::atoi(argv[4]) : 20;
        runConcurrentTest(host, port, numBots);
    } else if (testType == "packet") {
        runPacketOrderTest(host, port);
    }
    // All tests
    else if (testType == "all") {
        runAllTests(host, port);
    }
    else {
        std::cerr << "Unknown test type: " << testType << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
