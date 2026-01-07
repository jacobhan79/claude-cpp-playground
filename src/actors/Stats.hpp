#pragma once

#include <cstdint>
#include <string>
#include <cmath>

namespace mmorpg {

// Primary stats - base attributes that affect derived stats
struct PrimaryStats {
    int32_t strength = 10;      // Physical damage, carry capacity
    int32_t agility = 10;       // Attack speed, dodge chance, crit
    int32_t intelligence = 10;  // Magic damage, MP pool
    int32_t vitality = 10;      // HP pool, defense
    int32_t wisdom = 10;        // MP regen, magic defense
    int32_t luck = 10;          // Crit chance, drop rates
};

// Derived/computed stats - calculated from primary stats and level
struct DerivedStats {
    int32_t maxHp = 100;
    int32_t maxMp = 50;
    int32_t physicalAttack = 10;
    int32_t magicalAttack = 10;
    int32_t physicalDefense = 5;
    int32_t magicalDefense = 5;
    float criticalChance = 0.05f;    // 5%
    float criticalMultiplier = 1.5f;
    float dodgeChance = 0.05f;       // 5%
    float attackSpeed = 1.0f;        // Attacks per second
    float moveSpeed = 5.0f;          // Units per second
};

// Runtime stats - current values that change during gameplay
struct RuntimeStats {
    int32_t currentHp = 100;
    int32_t currentMp = 50;
};

// Stat calculator - derives stats from primary + level
class StatCalculator {
public:
    static DerivedStats calculate(const PrimaryStats& primary, int32_t level) {
        DerivedStats derived;

        // HP = base + (vitality * 10) + (level * 5)
        derived.maxHp = 100 + (primary.vitality * 10) + (level * 5);

        // MP = base + (intelligence * 5) + (wisdom * 3) + (level * 2)
        derived.maxMp = 50 + (primary.intelligence * 5) + (primary.wisdom * 3) + (level * 2);

        // Physical Attack = strength * 2 + (level / 2)
        derived.physicalAttack = primary.strength * 2 + (level / 2);

        // Magical Attack = intelligence * 2 + (level / 2)
        derived.magicalAttack = primary.intelligence * 2 + (level / 2);

        // Physical Defense = vitality + (strength / 2)
        derived.physicalDefense = primary.vitality + (primary.strength / 2);

        // Magical Defense = wisdom + (intelligence / 2)
        derived.magicalDefense = primary.wisdom + (primary.intelligence / 2);

        // Critical Chance = 5% + (luck * 0.5%) + (agility * 0.2%)
        derived.criticalChance = 0.05f + (primary.luck * 0.005f) + (primary.agility * 0.002f);
        derived.criticalChance = std::min(derived.criticalChance, 0.75f); // Cap at 75%

        // Critical Multiplier = 1.5 + (luck * 0.01)
        derived.criticalMultiplier = 1.5f + (primary.luck * 0.01f);

        // Dodge Chance = 5% + (agility * 0.3%)
        derived.dodgeChance = 0.05f + (primary.agility * 0.003f);
        derived.dodgeChance = std::min(derived.dodgeChance, 0.50f); // Cap at 50%

        // Attack Speed = 1.0 + (agility * 0.01)
        derived.attackSpeed = 1.0f + (primary.agility * 0.01f);

        // Move Speed = 5.0 + (agility * 0.05)
        derived.moveSpeed = 5.0f + (primary.agility * 0.05f);

        return derived;
    }

    // Calculate experience required for next level
    static int64_t experienceForLevel(int32_t level) {
        // Exponential growth: 100 * level^2
        return static_cast<int64_t>(100 * level * level);
    }
};

} // namespace mmorpg
