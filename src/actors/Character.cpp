#include "Character.hpp"
#include <iostream>

namespace mmorpg {

Character::Character(ActorId id, std::string name)
    : Actor(id, std::move(name)) {
    // Start with some skill points
    skillPoints_ = 3;
}

bool Character::learnSkill(SkillId skillId) {
    if (!canLearnSkill(skillId)) {
        return false;
    }

    // Get skill from database
    const auto* skill = SkillDatabase::instance().getSkill(skillId);
    if (!skill) return false;

    // Spend skill point
    skillPoints_--;

    // Learn the skill at level 1
    learnedSkills_.insert(skillId);
    skillLevels_[skillId] = 1;

    std::cout << name_ << " learned " << skill->getName() << "!" << std::endl;
    return true;
}

bool Character::upgradeSkill(SkillId skillId) {
    if (!canUpgradeSkill(skillId)) {
        return false;
    }

    // Spend skill point
    skillPoints_--;
    skillLevels_[skillId]++;

    const auto* skill = SkillDatabase::instance().getSkill(skillId);
    std::cout << name_ << " upgraded " << (skill ? skill->getName() : "skill")
              << " to level " << skillLevels_[skillId] << "!" << std::endl;
    return true;
}

bool Character::canLearnSkill(SkillId skillId) const {
    // Check skill points
    if (skillPoints_ <= 0) return false;

    // Check if already learned
    if (hasSkill(skillId)) return false;

    // Check skill tree prerequisites
    return skillTree_.canLearn(skillId, learnedSkills_, skillLevels_, level_);
}

bool Character::canUpgradeSkill(SkillId skillId) const {
    // Must have skill points
    if (skillPoints_ <= 0) return false;

    // Must have learned the skill
    if (!hasSkill(skillId)) return false;

    // Check max level
    const auto* skill = SkillDatabase::instance().getSkill(skillId);
    if (!skill) return false;

    auto it = skillLevels_.find(skillId);
    if (it == skillLevels_.end()) return false;

    return it->second < skill->getMaxLevel();
}

int32_t Character::getSkillLevel(SkillId skillId) const {
    auto it = skillLevels_.find(skillId);
    if (it == skillLevels_.end()) return 0;
    return it->second;
}

bool Character::hasSkill(SkillId skillId) const {
    return learnedSkills_.count(skillId) > 0;
}

std::vector<SkillId> Character::getAvailableSkills() const {
    return skillTree_.getAvailableSkills(learnedSkills_, level_);
}

void Character::onLevelUp() {
    // Call parent implementation
    Actor::onLevelUp();

    // Grant skill points
    skillPoints_ += SKILL_POINTS_PER_LEVEL;
    std::cout << name_ << " gained " << SKILL_POINTS_PER_LEVEL << " skill point(s)!" << std::endl;
}

bool Character::useSkill(SkillId skillId) {
    // Must have the skill
    if (!hasSkill(skillId)) {
        std::cout << "You haven't learned this skill!" << std::endl;
        return false;
    }

    // Get skill info
    const auto* skillBase = SkillDatabase::instance().getSkill(skillId);
    if (!skillBase) return false;

    // Create a copy with our level
    Skill skill = SkillDatabase::instance().getSkillCopy(skillId);
    skill.setLevel(getSkillLevel(skillId));

    // Check cooldown
    auto cdIt = skillCooldowns_.find(skillId);
    if (cdIt != skillCooldowns_.end()) {
        Tick cdRemaining = cdIt->second;
        if (cdRemaining > lastUpdateTick_) {
            std::cout << "Skill is on cooldown!" << std::endl;
            return false;
        }
    }

    // Check mana
    int32_t manaCost = skill.getScaledManaCost();
    if (!useMana(manaCost)) {
        std::cout << "Not enough mana! (Need " << manaCost << ")" << std::endl;
        return false;
    }

    // Set cooldown
    Tick cooldownTicks = static_cast<Tick>(skill.getScaledCooldown() * 1000);
    skillCooldowns_[skillId] = lastUpdateTick_ + cooldownTicks;

    std::cout << name_ << " uses " << skill.getName()
              << " (Level " << skill.getLevel() << ")!" << std::endl;
    return true;
}

float Character::getSkillCooldown(SkillId skillId) const {
    auto it = skillCooldowns_.find(skillId);
    if (it == skillCooldowns_.end()) return 0.0f;

    if (it->second <= lastUpdateTick_) return 0.0f;

    return static_cast<float>(it->second - lastUpdateTick_) / 1000.0f;
}

void Character::update(Tick currentTick) {
    Actor::update(currentTick);
    lastUpdateTick_ = currentTick;
}

} // namespace mmorpg
