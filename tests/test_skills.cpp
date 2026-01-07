#include <gtest/gtest.h>
#include "skills/Skill.hpp"
#include "skills/SkillTree.hpp"
#include "actors/Character.hpp"
#include "actors/ActorManager.hpp"

using namespace mmorpg;

class SkillTest : public ::testing::Test {
protected:
    void SetUp() override {
        SkillDatabase::instance().loadDefaultSkills();
    }

    void TearDown() override {
        SkillDatabase::instance().clear();
    }
};

TEST_F(SkillTest, SkillBuilderPattern) {
    Skill skill(100, "TestSkill");
    skill.withDescription("A test skill")
         .withType(SkillType::Active)
         .withTargetType(TargetType::SingleEnemy)
         .withManaCost(25)
         .withCooldown(5.0f)
         .withMaxLevel(3);

    EXPECT_EQ(skill.getId(), 100);
    EXPECT_EQ(skill.getName(), "TestSkill");
    EXPECT_EQ(skill.getDescription(), "A test skill");
    EXPECT_EQ(skill.getManaCost(), 25);
    EXPECT_FLOAT_EQ(skill.getCooldown(), 5.0f);
    EXPECT_EQ(skill.getMaxLevel(), 3);
}

TEST_F(SkillTest, SkillWithEffects) {
    Skill skill(101, "Fireball");
    skill.withEffect(DamageEffect{50, 1.2f, false});

    EXPECT_EQ(skill.getEffects().size(), 1);

    const auto& effect = skill.getEffects()[0];
    EXPECT_TRUE(std::holds_alternative<DamageEffect>(effect));

    auto* dmg = std::get_if<DamageEffect>(&effect);
    EXPECT_EQ(dmg->baseDamage, 50);
    EXPECT_FALSE(dmg->isPhysical);
}

TEST_F(SkillTest, SkillLevelUp) {
    Skill skill(102, "TestSkill");
    skill.withMaxLevel(5);
    skill.setLevel(1);

    EXPECT_EQ(skill.getLevel(), 1);
    EXPECT_TRUE(skill.canLevelUp());

    skill.levelUp();
    EXPECT_EQ(skill.getLevel(), 2);

    skill.setLevel(5);
    EXPECT_FALSE(skill.canLevelUp());
}

TEST_F(SkillTest, SkillDatabaseLoadsDefaults) {
    auto* slash = SkillDatabase::instance().getSkill(1);
    auto* fireball = SkillDatabase::instance().getSkill(2);
    auto* heal = SkillDatabase::instance().getSkill(3);

    EXPECT_NE(slash, nullptr);
    EXPECT_NE(fireball, nullptr);
    EXPECT_NE(heal, nullptr);

    EXPECT_EQ(slash->getName(), "Slash");
    EXPECT_EQ(fireball->getName(), "Fireball");
    EXPECT_EQ(heal->getName(), "Heal");
}

TEST_F(SkillTest, SkillDatabaseGetCopy) {
    Skill copy = SkillDatabase::instance().getSkillCopy(1);
    copy.setLevel(3);

    // Original should be unchanged
    const auto* original = SkillDatabase::instance().getSkill(1);
    EXPECT_EQ(original->getLevel(), 0);
    EXPECT_EQ(copy.getLevel(), 3);
}

class SkillTreeTest : public ::testing::Test {
protected:
    SkillTree tree;
    ActorManager manager;

    void SetUp() override {
        SkillDatabase::instance().loadDefaultSkills();

        // Build simple skill tree
        tree.addNode({1, {}, {4}, 1});      // Slash (tier 1)
        tree.addNode({2, {}, {5}, 1});      // Fireball (tier 1)
        tree.addNode({4, {1}, {7}, 2});     // Power Strike (tier 2, requires Slash)
        tree.addNode({5, {2}, {}, 2});      // Flame Wave (tier 2, requires Fireball)
        tree.addNode({7, {4}, {}, 3});      // Berserk (tier 3, requires Power Strike)
    }

    void TearDown() override {
        SkillDatabase::instance().clear();
    }
};

TEST_F(SkillTreeTest, GetAvailableSkillsInitial) {
    std::unordered_set<SkillId> learned;

    auto available = tree.getAvailableSkills(learned, 1);

    // Should only have tier 1 skills available
    EXPECT_EQ(available.size(), 2);
}

TEST_F(SkillTreeTest, GetAvailableSkillsAfterLearning) {
    std::unordered_set<SkillId> learned = {1};  // Learned Slash

    auto available = tree.getAvailableSkills(learned, 5);

    // Should have Fireball (tier 1) and Power Strike (tier 2)
    EXPECT_GE(available.size(), 1);
}

TEST_F(SkillTreeTest, CanLearnWithPrerequisites) {
    std::unordered_set<SkillId> learned = {1};  // Learned Slash
    std::unordered_map<SkillId, int32_t> levels = {{1, 2}};  // Slash at level 2

    bool canLearn = tree.canLearn(4, learned, levels, 5);  // Power Strike

    EXPECT_TRUE(canLearn);
}

TEST_F(SkillTreeTest, CannotLearnWithoutPrerequisites) {
    std::unordered_set<SkillId> learned;  // Nothing learned
    std::unordered_map<SkillId, int32_t> levels;

    bool canLearn = tree.canLearn(4, learned, levels, 5);  // Power Strike

    EXPECT_FALSE(canLearn);
}

TEST_F(SkillTreeTest, GetSkillsInTier) {
    auto tier1 = tree.getSkillsInTier(1);
    auto tier2 = tree.getSkillsInTier(2);
    auto tier3 = tree.getSkillsInTier(3);

    EXPECT_EQ(tier1.size(), 2);
    EXPECT_EQ(tier2.size(), 2);
    EXPECT_EQ(tier3.size(), 1);
}

class CharacterSkillTest : public ::testing::Test {
protected:
    ActorManager manager;
    SkillTree tree;

    void SetUp() override {
        SkillDatabase::instance().loadDefaultSkills();

        tree.addNode({1, {}, {}, 1});  // Slash
        tree.addNode({2, {}, {}, 1});  // Fireball
    }

    void TearDown() override {
        SkillDatabase::instance().clear();
    }
};

TEST_F(CharacterSkillTest, CharacterLearnSkill) {
    auto character = manager.createActor<Character>("Hero");
    character->setSkillTree(tree);

    bool learned = character->learnSkill(1);

    EXPECT_TRUE(learned);
    EXPECT_TRUE(character->hasSkill(1));
    EXPECT_EQ(character->getSkillLevel(1), 1);
}

TEST_F(CharacterSkillTest, CharacterUpgradeSkill) {
    auto character = manager.createActor<Character>("Hero");
    character->setSkillTree(tree);
    character->learnSkill(1);

    bool upgraded = character->upgradeSkill(1);

    EXPECT_TRUE(upgraded);
    EXPECT_EQ(character->getSkillLevel(1), 2);
}

TEST_F(CharacterSkillTest, CannotLearnWithoutSkillPoints) {
    auto character = manager.createActor<Character>("Hero");
    character->setSkillTree(tree);

    // Use all skill points
    character->learnSkill(1);
    character->learnSkill(2);
    character->upgradeSkill(1);

    // Should have 0 skill points now
    EXPECT_EQ(character->getSkillPoints(), 0);

    bool canLearn = character->canLearnSkill(3);
    EXPECT_FALSE(canLearn);
}

TEST_F(CharacterSkillTest, LevelUpGrantsSkillPoints) {
    auto character = manager.createActor<Character>("Hero");
    int32_t initialPoints = character->getSkillPoints();

    character->gainExperience(500);  // Should level up

    EXPECT_GT(character->getSkillPoints(), initialPoints);
}
