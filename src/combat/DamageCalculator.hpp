#pragma once

#include "../actors/Actor.hpp"
#include <memory>
#include <random>

namespace mmorpg {

// Forward declaration
class Skill;

// Result of damage calculation
struct DamageResult {
    int32_t rawDamage = 0;      // Before defense
    int32_t finalDamage = 0;   // After defense
    bool isCritical = false;
    bool isDodged = false;
    bool isBlocked = false;
    bool isPhysical = true;
};

// Strategy pattern: interface for damage formulas
class DamageFormula {
public:
    virtual ~DamageFormula() = default;

    virtual DamageResult calculate(
        const Actor& attacker,
        const Actor& defender,
        bool isPhysical = true,
        int32_t bonusDamage = 0
    ) = 0;
};

// Standard MMORPG-style damage formula
class StandardDamageFormula : public DamageFormula {
public:
    StandardDamageFormula();

    DamageResult calculate(
        const Actor& attacker,
        const Actor& defender,
        bool isPhysical = true,
        int32_t bonusDamage = 0
    ) override;

    // Set random seed for testing
    void setSeed(unsigned int seed) { rng_.seed(seed); }

private:
    bool rollCritical(float critChance);
    bool rollDodge(float dodgeChance);

    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_{0.0f, 1.0f};
};

// Facade for easy damage calculation
class DamageCalculator {
public:
    explicit DamageCalculator(std::unique_ptr<DamageFormula> formula = nullptr);

    // Basic attack (uses attacker's physical/magical attack)
    DamageResult calculateBasicAttack(
        const Actor& attacker,
        const Actor& defender,
        bool isPhysical = true
    );

    // Skill damage (adds skill's base damage)
    DamageResult calculateSkillDamage(
        const Actor& attacker,
        const Actor& defender,
        int32_t skillBaseDamage,
        bool isPhysical = true
    );

    // Swap damage formula (for different game modes)
    void setFormula(std::unique_ptr<DamageFormula> formula);

    // Get formula for configuration
    DamageFormula* getFormula() { return formula_.get(); }

private:
    std::unique_ptr<DamageFormula> formula_;
};

} // namespace mmorpg
