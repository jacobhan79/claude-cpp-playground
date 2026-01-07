#include "CombatSystem.hpp"
#include <iostream>

namespace mmorpg {

CombatSystem::CombatSystem(ActorManager& actors, EventBus& events)
    : actors_(actors)
    , events_(events) {
}

void CombatSystem::processAction(const CombatAction& action) {
    if (!canPerformAction(action)) {
        return;
    }

    std::visit([this](auto&& act) {
        using T = std::decay_t<decltype(act)>;
        if constexpr (std::is_same_v<T, BasicAttack>) {
            handleBasicAttack(act);
        } else if constexpr (std::is_same_v<T, SkillAttack>) {
            handleSkillAttack(act);
        } else if constexpr (std::is_same_v<T, AreaSkill>) {
            handleAreaSkill(act);
        } else if constexpr (std::is_same_v<T, SelfSkill>) {
            handleSelfSkill(act);
        }
    }, action);
}

DamageResult CombatSystem::handleBasicAttack(const BasicAttack& attack) {
    auto attacker = actors_.getActor(attack.attacker);
    auto target = actors_.getActor(attack.target);

    if (!attacker || !target) {
        return {};
    }

    // Calculate damage
    auto result = damageCalc_.calculateBasicAttack(*attacker, *target, attack.isPhysical);

    // Apply damage if not dodged
    if (!result.isDodged) {
        target->takeDamage(result.finalDamage);

        // Publish damage event
        DamageEvent event{
            attack.attacker,
            attack.target,
            result.finalDamage,
            result.isCritical,
            result.isPhysical
        };
        events_.publish(event);

        // Check for death
        if (!target->isAlive()) {
            DeathEvent deathEvent{attack.target, attack.attacker};
            events_.publish(deathEvent);
        }
    }

    return result;
}

void CombatSystem::handleSkillAttack(const SkillAttack& attack) {
    auto caster = actors_.getActor(attack.caster);
    auto target = actors_.getActor(attack.target);

    if (!caster || !target) {
        return;
    }

    // For now, treat skill attacks as enhanced basic attacks
    // Full skill system will be implemented in Phase 3
    int32_t skillBonus = 20;  // Placeholder bonus damage

    auto result = damageCalc_.calculateSkillDamage(*caster, *target, skillBonus, true);

    if (!result.isDodged) {
        target->takeDamage(result.finalDamage);

        // Publish events
        SkillUsedEvent skillEvent{attack.caster, attack.skill, attack.target};
        events_.publish(skillEvent);

        DamageEvent damageEvent{
            attack.caster,
            attack.target,
            result.finalDamage,
            result.isCritical,
            result.isPhysical
        };
        events_.publish(damageEvent);

        if (!target->isAlive()) {
            DeathEvent deathEvent{attack.target, attack.caster};
            events_.publish(deathEvent);
        }
    }
}

void CombatSystem::handleAreaSkill(const AreaSkill& attack) {
    // Placeholder for area skill handling
    // Would need position system to find actors in radius
    SkillUsedEvent event{attack.caster, attack.skill, INVALID_ACTOR_ID};
    events_.publish(event);
}

void CombatSystem::handleSelfSkill(const SelfSkill& action) {
    auto caster = actors_.getActor(action.caster);
    if (!caster) return;

    // Placeholder: self skills will be properly implemented with skill system
    SkillUsedEvent event{action.caster, action.skill, action.caster};
    events_.publish(event);
}

bool CombatSystem::canPerformAction(const CombatAction& action) const {
    return std::visit([this](auto&& act) -> bool {
        using T = std::decay_t<decltype(act)>;

        if constexpr (std::is_same_v<T, BasicAttack>) {
            return validateAttacker(act.attacker) && validateTarget(act.target);
        } else if constexpr (std::is_same_v<T, SkillAttack>) {
            return validateAttacker(act.caster) && validateTarget(act.target);
        } else if constexpr (std::is_same_v<T, AreaSkill>) {
            return validateAttacker(act.caster);
        } else if constexpr (std::is_same_v<T, SelfSkill>) {
            return validateAttacker(act.caster);
        }
        return false;
    }, action);
}

bool CombatSystem::validateAttacker(ActorId id) const {
    auto actor = actors_.getActor(id);
    return actor && actor->isAlive();
}

bool CombatSystem::validateTarget(ActorId id) const {
    auto actor = actors_.getActor(id);
    return actor && actor->isAlive();
}

} // namespace mmorpg
