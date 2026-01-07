#include <gtest/gtest.h>
#include "actors/Stats.hpp"

using namespace mmorpg;

class StatsTest : public ::testing::Test {
protected:
    PrimaryStats defaultStats;
};

TEST_F(StatsTest, DefaultPrimaryStats) {
    EXPECT_EQ(defaultStats.strength, 10);
    EXPECT_EQ(defaultStats.agility, 10);
    EXPECT_EQ(defaultStats.intelligence, 10);
    EXPECT_EQ(defaultStats.vitality, 10);
    EXPECT_EQ(defaultStats.wisdom, 10);
    EXPECT_EQ(defaultStats.luck, 10);
}

TEST_F(StatsTest, StatCalculatorBasicHP) {
    PrimaryStats stats;
    stats.vitality = 10;

    auto derived = StatCalculator::calculate(stats, 1);
    // HP = 100 + (vitality * 10) + (level * 5) = 100 + 100 + 5 = 205
    EXPECT_EQ(derived.maxHp, 205);
}

TEST_F(StatsTest, StatCalculatorBasicMP) {
    PrimaryStats stats;
    stats.intelligence = 10;
    stats.wisdom = 10;

    auto derived = StatCalculator::calculate(stats, 1);
    // MP = 50 + (int * 5) + (wis * 3) + (level * 2) = 50 + 50 + 30 + 2 = 132
    EXPECT_EQ(derived.maxMp, 132);
}

TEST_F(StatsTest, StatCalculatorPhysicalAttack) {
    PrimaryStats stats;
    stats.strength = 20;

    auto derived = StatCalculator::calculate(stats, 1);
    // Physical ATK = strength * 2 + (level / 2) = 40 + 0 = 40
    EXPECT_EQ(derived.physicalAttack, 40);
}

TEST_F(StatsTest, StatCalculatorMagicalAttack) {
    PrimaryStats stats;
    stats.intelligence = 20;

    auto derived = StatCalculator::calculate(stats, 1);
    // Magical ATK = intelligence * 2 + (level / 2) = 40 + 0 = 40
    EXPECT_EQ(derived.magicalAttack, 40);
}

TEST_F(StatsTest, StatCalculatorCritChanceCapped) {
    PrimaryStats stats;
    stats.luck = 200;  // Very high luck
    stats.agility = 200;

    auto derived = StatCalculator::calculate(stats, 1);
    // Crit chance should be capped at 75%
    EXPECT_LE(derived.criticalChance, 0.75f);
}

TEST_F(StatsTest, StatCalculatorDodgeChanceCapped) {
    PrimaryStats stats;
    stats.agility = 200;  // Very high agility

    auto derived = StatCalculator::calculate(stats, 1);
    // Dodge chance should be capped at 50%
    EXPECT_LE(derived.dodgeChance, 0.50f);
}

TEST_F(StatsTest, ExperienceForLevelScaling) {
    // Level 1 -> 2: 100 * 1 * 1 = 100
    EXPECT_EQ(StatCalculator::experienceForLevel(1), 100);

    // Level 2 -> 3: 100 * 2 * 2 = 400
    EXPECT_EQ(StatCalculator::experienceForLevel(2), 400);

    // Level 10 -> 11: 100 * 10 * 10 = 10000
    EXPECT_EQ(StatCalculator::experienceForLevel(10), 10000);
}

TEST_F(StatsTest, HighLevelStatsScaling) {
    PrimaryStats stats;
    stats.vitality = 50;
    stats.intelligence = 50;

    auto derivedLevel1 = StatCalculator::calculate(stats, 1);
    auto derivedLevel50 = StatCalculator::calculate(stats, 50);

    // Higher level should have more HP and MP
    EXPECT_GT(derivedLevel50.maxHp, derivedLevel1.maxHp);
    EXPECT_GT(derivedLevel50.maxMp, derivedLevel1.maxMp);
}
