#pragma once

#include "CombatAction.hpp"
#include "DamageCalculator.hpp"
#include "../actors/ActorManager.hpp"
#include "../core/EventBus.hpp"
#include <memory>

namespace mmorpg {

class CombatSystem {
public:
    CombatSystem(ActorManager& actors, EventBus& events);

    // Process a combat action
    void processAction(const CombatAction& action);

    // Individual action handlers (public for testing)
    DamageResult handleBasicAttack(const BasicAttack& attack);
    void handleSkillAttack(const SkillAttack& attack);
    void handleAreaSkill(const AreaSkill& attack);
    void handleSelfSkill(const SelfSkill& action);

    // Check if attack is valid (both actors alive, etc.)
    bool canPerformAction(const CombatAction& action) const;

    // Get damage calculator for configuration
    DamageCalculator& getDamageCalculator() { return damageCalc_; }

private:
    ActorManager& actors_;
    EventBus& events_;
    DamageCalculator damageCalc_;

    // Validate actors exist and are alive
    bool validateAttacker(ActorId id) const;
    bool validateTarget(ActorId id) const;
};

} // namespace mmorpg
