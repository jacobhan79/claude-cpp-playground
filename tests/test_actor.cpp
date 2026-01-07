#include <gtest/gtest.h>
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"

using namespace mmorpg;

class ActorTest : public ::testing::Test {
protected:
    ActorManager manager;

    void SetUp() override {
        // Fresh manager for each test
    }
};

TEST_F(ActorTest, CreateActor) {
    auto actor = manager.createActor<Actor>("TestActor");

    EXPECT_NE(actor, nullptr);
    EXPECT_EQ(actor->getName(), "TestActor");
    EXPECT_EQ(actor->getLevel(), 1);
    EXPECT_TRUE(actor->isAlive());
}

TEST_F(ActorTest, ActorIdUnique) {
    auto actor1 = manager.createActor<Actor>("Actor1");
    auto actor2 = manager.createActor<Actor>("Actor2");

    EXPECT_NE(actor1->getId(), actor2->getId());
}

TEST_F(ActorTest, TakeDamage) {
    auto actor = manager.createActor<Actor>("TestActor");
    int32_t initialHp = actor->getRuntimeStats().currentHp;

    int32_t damageDealt = actor->takeDamage(50);

    EXPECT_EQ(damageDealt, 50);
    EXPECT_EQ(actor->getRuntimeStats().currentHp, initialHp - 50);
    EXPECT_TRUE(actor->isAlive());
}

TEST_F(ActorTest, TakeFatalDamage) {
    auto actor = manager.createActor<Actor>("TestActor");
    int32_t maxHp = actor->getDerivedStats().maxHp;

    actor->takeDamage(maxHp + 100);

    EXPECT_FALSE(actor->isAlive());
    EXPECT_EQ(actor->getRuntimeStats().currentHp, 0);
}

TEST_F(ActorTest, Heal) {
    auto actor = manager.createActor<Actor>("TestActor");
    actor->takeDamage(100);
    int32_t damagedHp = actor->getRuntimeStats().currentHp;

    int32_t healAmount = actor->heal(50);

    EXPECT_EQ(healAmount, 50);
    EXPECT_EQ(actor->getRuntimeStats().currentHp, damagedHp + 50);
}

TEST_F(ActorTest, HealCannotExceedMax) {
    auto actor = manager.createActor<Actor>("TestActor");
    int32_t maxHp = actor->getDerivedStats().maxHp;

    // Take small damage
    actor->takeDamage(10);

    // Try to heal more than missing
    int32_t healAmount = actor->heal(100);

    EXPECT_EQ(healAmount, 10);  // Should only heal 10
    EXPECT_EQ(actor->getRuntimeStats().currentHp, maxHp);
}

TEST_F(ActorTest, UseMana) {
    auto actor = manager.createActor<Actor>("TestActor");
    int32_t initialMp = actor->getRuntimeStats().currentMp;

    bool success = actor->useMana(30);

    EXPECT_TRUE(success);
    EXPECT_EQ(actor->getRuntimeStats().currentMp, initialMp - 30);
}

TEST_F(ActorTest, UseManaInsufficent) {
    auto actor = manager.createActor<Actor>("TestActor");
    int32_t maxMp = actor->getDerivedStats().maxMp;
    int32_t initialMp = actor->getRuntimeStats().currentMp;

    bool success = actor->useMana(maxMp + 100);

    EXPECT_FALSE(success);
    EXPECT_EQ(actor->getRuntimeStats().currentMp, initialMp);  // Unchanged
}

TEST_F(ActorTest, GainExperienceAndLevelUp) {
    auto actor = manager.createActor<Actor>("TestActor");
    EXPECT_EQ(actor->getLevel(), 1);

    // Need 400 exp for level 2 (formula: 100 * level^2)
    actor->gainExperience(400);

    EXPECT_EQ(actor->getLevel(), 2);
}

TEST_F(ActorTest, MultiLevelUp) {
    auto actor = manager.createActor<Actor>("TestActor");

    // Gain enough exp for multiple levels
    // Level 2: 100, Level 3: 400, Level 4: 900, Level 5: 1600
    actor->gainExperience(2000);

    EXPECT_GE(actor->getLevel(), 4);
}

TEST_F(ActorTest, ModifyPrimaryStat) {
    auto actor = manager.createActor<Actor>("TestActor");
    int32_t initialStr = actor->getPrimaryStats().strength;

    actor->modifyPrimaryStat("strength", 10);

    EXPECT_EQ(actor->getPrimaryStats().strength, initialStr + 10);
}

TEST_F(ActorTest, ActorManagerGetActor) {
    auto actor = manager.createActor<Actor>("TestActor");
    ActorId id = actor->getId();

    auto retrieved = manager.getActor(id);

    EXPECT_EQ(retrieved, actor);
}

TEST_F(ActorTest, ActorManagerRemoveActor) {
    auto actor = manager.createActor<Actor>("TestActor");
    ActorId id = actor->getId();

    bool removed = manager.removeActor(id);

    EXPECT_TRUE(removed);
    EXPECT_EQ(manager.getActor(id), nullptr);
}

TEST_F(ActorTest, ActorManagerGetLivingActors) {
    auto actor1 = manager.createActor<Actor>("Actor1");
    auto actor2 = manager.createActor<Actor>("Actor2");

    // Kill actor1
    actor1->takeDamage(9999);

    auto living = manager.getLivingActors();

    EXPECT_EQ(living.size(), 1);
    EXPECT_EQ(living[0], actor2);
}

TEST_F(ActorTest, HpPercentage) {
    auto actor = manager.createActor<Actor>("TestActor");

    EXPECT_FLOAT_EQ(actor->getHpPercent(), 1.0f);

    actor->takeDamage(actor->getDerivedStats().maxHp / 2);

    // Use EXPECT_NEAR due to integer division rounding
    EXPECT_NEAR(actor->getHpPercent(), 0.5f, 0.01f);
}
