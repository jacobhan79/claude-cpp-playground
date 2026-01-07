#include "Skill.hpp"
#include <cmath>

namespace mmorpg {

Skill::Skill(SkillId id, std::string name)
    : id_(id)
    , name_(std::move(name)) {
}

int32_t Skill::getScaledManaCost() const {
    // Mana cost increases slightly with level
    // Level 1: base cost, Level 5: base * 1.4
    float multiplier = 1.0f + (level_ - 1) * 0.1f;
    return static_cast<int32_t>(manaCost_ * multiplier);
}

float Skill::getScaledCooldown() const {
    // Cooldown decreases with level
    // Level 1: base cooldown, Level 5: base * 0.7
    float multiplier = 1.0f - (level_ - 1) * 0.075f;
    return cooldown_ * std::max(0.5f, multiplier);
}

int32_t Skill::getScaledDamage() const {
    // Find first damage effect and scale it
    for (const auto& effect : effects_) {
        if (auto* dmg = std::get_if<DamageEffect>(&effect)) {
            // Damage increases with level
            // Level 1: base damage, Level 5: base * 2.0
            float multiplier = 1.0f + (level_ - 1) * 0.25f;
            return static_cast<int32_t>(dmg->baseDamage * multiplier);
        }
    }
    return 0;
}

} // namespace mmorpg
