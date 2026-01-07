#include <gtest/gtest.h>
#include "server/GameServer.hpp"
#include <fstream>
#include <filesystem>

using namespace mmorpg;

class ServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary config file for testing
        testConfigPath = std::filesystem::temp_directory_path() / "test_server_config.json";
        std::ofstream configFile(testConfigPath);
        configFile << R"({
            "server": {
                "port": 9999,
                "tick_rate": 30
            },
            "network": {
                "max_connections": 50,
                "timeout_ms": 15000
            },
            "game": {
                "starting_level": 5,
                "starting_skill_points": 10,
                "exp_multiplier": 2.0
            }
        })";
        configFile.close();
    }

    void TearDown() override {
        if (std::filesystem::exists(testConfigPath)) {
            std::filesystem::remove(testConfigPath);
        }
    }

    std::filesystem::path testConfigPath;
};

TEST_F(ServerTest, DefaultConfig) {
    GameServer::Config config;

    EXPECT_EQ(config.port, 7777);
    EXPECT_EQ(config.tickRate, 20);
    EXPECT_EQ(config.maxConnections, 100);
    EXPECT_EQ(config.timeoutMs, 30000);
    EXPECT_EQ(config.startingLevel, 1);
    EXPECT_EQ(config.startingSkillPoints, 3);
    EXPECT_FLOAT_EQ(config.expMultiplier, 1.0f);
}

TEST_F(ServerTest, LoadConfigFromFile) {
    auto config = GameServer::Config::loadFromFile(testConfigPath.string());

    EXPECT_EQ(config.port, 9999);
    EXPECT_EQ(config.tickRate, 30);
    EXPECT_EQ(config.maxConnections, 50);
    EXPECT_EQ(config.timeoutMs, 15000);
    EXPECT_EQ(config.startingLevel, 5);
    EXPECT_EQ(config.startingSkillPoints, 10);
    EXPECT_FLOAT_EQ(config.expMultiplier, 2.0f);
}

TEST_F(ServerTest, LoadConfigFromNonExistentFile) {
    auto config = GameServer::Config::loadFromFile("/nonexistent/path.json");

    // Should return default config when file doesn't exist
    EXPECT_EQ(config.port, 7777);
    EXPECT_EQ(config.tickRate, 20);
}

TEST_F(ServerTest, LoadConfigWithPartialData) {
    // Create a config file with only some fields
    std::filesystem::path partialConfigPath = std::filesystem::temp_directory_path() / "partial_config.json";
    std::ofstream configFile(partialConfigPath);
    configFile << R"({
        "server": {
            "port": 8888
        }
    })";
    configFile.close();

    auto config = GameServer::Config::loadFromFile(partialConfigPath.string());

    // Specified value should be loaded
    EXPECT_EQ(config.port, 8888);
    // Non-specified values should be defaults
    EXPECT_EQ(config.tickRate, 20);
    EXPECT_EQ(config.maxConnections, 100);

    std::filesystem::remove(partialConfigPath);
}

TEST_F(ServerTest, ServerInitialization) {
    GameServer::Config config;
    config.port = 17777;  // Use non-standard port to avoid conflicts

    GameServer server(config);

    EXPECT_FALSE(server.isRunning());
    EXPECT_TRUE(server.initialize());
    EXPECT_TRUE(server.isRunning());

    server.shutdown();
    EXPECT_FALSE(server.isRunning());
}

TEST_F(ServerTest, ActorManagerAccess) {
    GameServer::Config config;
    config.port = 17778;

    GameServer server(config);
    ASSERT_TRUE(server.initialize());

    auto& actorManager = server.getActorManager();
    EXPECT_EQ(actorManager.getActorCount(), 0);

    server.shutdown();
}

TEST_F(ServerTest, EventBusAccess) {
    GameServer::Config config;
    config.port = 17779;

    GameServer server(config);
    ASSERT_TRUE(server.initialize());

    auto& eventBus = server.getEventBus();
    EXPECT_EQ(eventBus.getSubscriberCount(), 2);  // GameServer subscribes to damage and death events

    server.shutdown();
}
