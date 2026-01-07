#pragma once

#include "../core/Types.hpp"
#include <variant>
#include <chrono>
#include <string>

namespace mmorpg {

// Direct damage effect
struct DamageEffect {
    int32_t baseDamage = 0;
    float statScaling = 1.0f;   // Multiplier from attacker's ATK stat
    bool isPhysical = true;     // Physical or magical damage
};

// Healing effect
struct HealEffect {
    int32_t baseHeal = 0;
    float statScaling = 1.0f;   // Multiplier from caster's relevant stat
};

// Buff effect (temporary stat increase)
struct BuffEffect {
    std::string statName;       // Which stat to buff (e.g., "strength")
    int32_t flatBonus = 0;      // +X to stat
    float percentBonus = 0.0f;  // +X% to stat
    float duration = 10.0f;     // Duration in seconds
};

// Debuff effect (temporary stat decrease)
struct DebuffEffect {
    std::string statName;
    int32_t flatPenalty = 0;
    float percentPenalty = 0.0f;
    float duration = 10.0f;
};

// Damage over time effect
struct DotEffect {
    int32_t damagePerTick = 0;
    float duration = 10.0f;       // Total duration in seconds
    float tickInterval = 1.0f;    // Seconds between ticks
    bool isPhysical = false;      // Usually magical (poison, burn, etc.)
};

// Heal over time effect
struct HotEffect {
    int32_t healPerTick = 0;
    float duration = 10.0f;
    float tickInterval = 1.0f;
};

// Mana restore effect
struct ManaRestoreEffect {
    int32_t amount = 0;
    float statScaling = 0.0f;
};

// Shield effect (absorbs damage)
struct ShieldEffect {
    int32_t amount = 0;
    float duration = 10.0f;
    bool absorbsPhysical = true;
    bool absorbsMagical = true;
};

// Unified skill effect type using variant
using SkillEffect = std::variant<
    DamageEffect,
    HealEffect,
    BuffEffect,
    DebuffEffect,
    DotEffect,
    HotEffect,
    ManaRestoreEffect,
    ShieldEffect
>;

// Helper to get effect type name
inline const char* getEffectTypeName(const SkillEffect& effect) {
    return std::visit([](auto&& e) -> const char* {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, DamageEffect>) return "Damage";
        else if constexpr (std::is_same_v<T, HealEffect>) return "Heal";
        else if constexpr (std::is_same_v<T, BuffEffect>) return "Buff";
        else if constexpr (std::is_same_v<T, DebuffEffect>) return "Debuff";
        else if constexpr (std::is_same_v<T, DotEffect>) return "DoT";
        else if constexpr (std::is_same_v<T, HotEffect>) return "HoT";
        else if constexpr (std::is_same_v<T, ManaRestoreEffect>) return "ManaRestore";
        else if constexpr (std::is_same_v<T, ShieldEffect>) return "Shield";
        else return "Unknown";
    }, effect);
}

} // namespace mmorpg
