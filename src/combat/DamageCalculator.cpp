#include "DamageCalculator.hpp"
#include <algorithm>
#include <chrono>

namespace mmorpg {

// StandardDamageFormula implementation

StandardDamageFormula::StandardDamageFormula()
    : rng_(static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count())) {
}

DamageResult StandardDamageFormula::calculate(
    const Actor& attacker,
    const Actor& defender,
    bool isPhysical,
    int32_t bonusDamage
) {
    DamageResult result;
    result.isPhysical = isPhysical;

    const auto& attackerStats = attacker.getDerivedStats();
    const auto& defenderStats = defender.getDerivedStats();

    // Check for dodge first
    if (rollDodge(defenderStats.dodgeChance)) {
        result.isDodged = true;
        return result;
    }

    // Calculate base damage
    int32_t attack = isPhysical ? attackerStats.physicalAttack : attackerStats.magicalAttack;
    int32_t defense = isPhysical ? defenderStats.physicalDefense : defenderStats.magicalDefense;

    // Raw damage = attack + bonus
    result.rawDamage = attack + bonusDamage;

    // Check for critical hit
    if (rollCritical(attackerStats.criticalChance)) {
        result.isCritical = true;
        result.rawDamage = static_cast<int32_t>(result.rawDamage * attackerStats.criticalMultiplier);
    }

    // Apply defense: finalDamage = rawDamage * (100 / (100 + defense))
    // This gives diminishing returns on defense
    float damageReduction = 100.0f / (100.0f + defense);
    result.finalDamage = static_cast<int32_t>(result.rawDamage * damageReduction);

    // Minimum 1 damage if we hit
    result.finalDamage = std::max(1, result.finalDamage);

    return result;
}

bool StandardDamageFormula::rollCritical(float critChance) {
    return dist_(rng_) < critChance;
}

bool StandardDamageFormula::rollDodge(float dodgeChance) {
    return dist_(rng_) < dodgeChance;
}

// DamageCalculator implementation

DamageCalculator::DamageCalculator(std::unique_ptr<DamageFormula> formula) {
    if (formula) {
        formula_ = std::move(formula);
    } else {
        formula_ = std::make_unique<StandardDamageFormula>();
    }
}

DamageResult DamageCalculator::calculateBasicAttack(
    const Actor& attacker,
    const Actor& defender,
    bool isPhysical
) {
    return formula_->calculate(attacker, defender, isPhysical, 0);
}

DamageResult DamageCalculator::calculateSkillDamage(
    const Actor& attacker,
    const Actor& defender,
    int32_t skillBaseDamage,
    bool isPhysical
) {
    return formula_->calculate(attacker, defender, isPhysical, skillBaseDamage);
}

void DamageCalculator::setFormula(std::unique_ptr<DamageFormula> formula) {
    formula_ = std::move(formula);
}

} // namespace mmorpg
