#pragma once

#include "../core/Types.hpp"
#include <variant>

namespace mmorpg {

// Basic auto-attack
struct BasicAttack {
    ActorId attacker;
    ActorId target;
    bool isPhysical = true;
};

// Skill-based attack on single target
struct SkillAttack {
    ActorId caster;
    ActorId target;
    SkillId skill;
};

// Area of effect skill
struct AreaSkill {
    ActorId caster;
    float centerX;
    float centerY;
    float radius;
    SkillId skill;
};

// Self-targeted skill (buff, heal, etc.)
struct SelfSkill {
    ActorId caster;
    SkillId skill;
};

// Unified combat action type
using CombatAction = std::variant<
    BasicAttack,
    SkillAttack,
    AreaSkill,
    SelfSkill
>;

} // namespace mmorpg
