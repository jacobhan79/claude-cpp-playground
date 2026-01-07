#pragma once

#include "Types.hpp"
#include <variant>
#include <cstdint>

namespace mmorpg {

// Event types for decoupled communication between systems

struct DamageEvent {
    ActorId attacker;
    ActorId target;
    int32_t damage;
    bool isCritical;
    bool isPhysical;  // true = physical, false = magical
};

struct DeathEvent {
    ActorId actor;
    ActorId killer;  // INVALID_ACTOR_ID if no killer (e.g., environmental)
};

struct HealEvent {
    ActorId healer;
    ActorId target;
    int32_t amount;
};

struct SkillUsedEvent {
    ActorId caster;
    SkillId skill;
    ActorId target;  // INVALID_ACTOR_ID for self/area skills
};

struct LevelUpEvent {
    ActorId actor;
    int32_t oldLevel;
    int32_t newLevel;
};

struct ManaUsedEvent {
    ActorId actor;
    int32_t amount;
    SkillId skill;  // INVALID_SKILL_ID if not from skill
};

struct BuffAppliedEvent {
    ActorId source;
    ActorId target;
    SkillId buffId;
    float duration;
};

struct BuffRemovedEvent {
    ActorId target;
    SkillId buffId;
};

// Variant-based event type (type-safe union)
using GameEvent = std::variant<
    DamageEvent,
    DeathEvent,
    HealEvent,
    SkillUsedEvent,
    LevelUpEvent,
    ManaUsedEvent,
    BuffAppliedEvent,
    BuffRemovedEvent
>;

// Helper to get event type name for debugging
inline const char* getEventTypeName(const GameEvent& event) {
    return std::visit([](auto&& e) -> const char* {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, DamageEvent>) return "DamageEvent";
        else if constexpr (std::is_same_v<T, DeathEvent>) return "DeathEvent";
        else if constexpr (std::is_same_v<T, HealEvent>) return "HealEvent";
        else if constexpr (std::is_same_v<T, SkillUsedEvent>) return "SkillUsedEvent";
        else if constexpr (std::is_same_v<T, LevelUpEvent>) return "LevelUpEvent";
        else if constexpr (std::is_same_v<T, ManaUsedEvent>) return "ManaUsedEvent";
        else if constexpr (std::is_same_v<T, BuffAppliedEvent>) return "BuffAppliedEvent";
        else if constexpr (std::is_same_v<T, BuffRemovedEvent>) return "BuffRemovedEvent";
        else return "Unknown";
    }, event);
}

} // namespace mmorpg
