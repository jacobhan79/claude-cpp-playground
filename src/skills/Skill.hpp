#pragma once

#include "../core/Types.hpp"
#include "SkillEffect.hpp"
#include <string>
#include <vector>

namespace mmorpg {

enum class SkillType {
    Active,     // Must be activated manually
    Passive,    // Always active once learned
    Toggle      // Can be turned on/off
};

enum class TargetType {
    Self,           // Targets caster only
    SingleEnemy,    // Single enemy target
    SingleAlly,     // Single ally target
    AreaEnemy,      // Area damage to enemies
    AreaAlly,       // Area buff/heal to allies
    AreaAll         // Affects all in area
};

// Requirements to learn a skill
struct SkillRequirement {
    SkillId prerequisiteSkill = INVALID_SKILL_ID;  // Must have this skill first
    int32_t prerequisiteLevel = 0;  // Required level in prerequisite skill
    int32_t requiredCharLevel = 1;  // Required character level
};

class Skill {
public:
    Skill(SkillId id, std::string name);

    // Accessors
    SkillId getId() const { return id_; }
    const std::string& getName() const { return name_; }
    const std::string& getDescription() const { return description_; }
    SkillType getType() const { return type_; }
    TargetType getTargetType() const { return targetType_; }

    // Cost and cooldown
    int32_t getManaCost() const { return manaCost_; }
    float getCooldown() const { return cooldown_; }
    float getRange() const { return range_; }

    // Skill level (upgradeable through skill tree)
    int32_t getLevel() const { return level_; }
    int32_t getMaxLevel() const { return maxLevel_; }
    void setLevel(int32_t level) { level_ = std::min(level, maxLevel_); }
    bool canLevelUp() const { return level_ < maxLevel_; }
    void levelUp() { if (canLevelUp()) level_++; }

    // Effects
    const std::vector<SkillEffect>& getEffects() const { return effects_; }
    void addEffect(SkillEffect effect) { effects_.push_back(std::move(effect)); }

    // Requirements
    const SkillRequirement& getRequirement() const { return requirement_; }

    // Calculate scaled values based on skill level
    int32_t getScaledManaCost() const;
    float getScaledCooldown() const;
    int32_t getScaledDamage() const;  // For first damage effect

    // Builder pattern for fluent construction
    Skill& withDescription(std::string desc) {
        description_ = std::move(desc);
        return *this;
    }
    Skill& withType(SkillType type) {
        type_ = type;
        return *this;
    }
    Skill& withTargetType(TargetType target) {
        targetType_ = target;
        return *this;
    }
    Skill& withManaCost(int32_t cost) {
        manaCost_ = cost;
        return *this;
    }
    Skill& withCooldown(float cd) {
        cooldown_ = cd;
        return *this;
    }
    Skill& withRange(float r) {
        range_ = r;
        return *this;
    }
    Skill& withMaxLevel(int32_t max) {
        maxLevel_ = max;
        return *this;
    }
    Skill& withRequirement(SkillRequirement req) {
        requirement_ = req;
        return *this;
    }
    Skill& withEffect(SkillEffect effect) {
        effects_.push_back(std::move(effect));
        return *this;
    }

private:
    SkillId id_;
    std::string name_;
    std::string description_;
    SkillType type_ = SkillType::Active;
    TargetType targetType_ = TargetType::SingleEnemy;

    int32_t manaCost_ = 10;
    float cooldown_ = 1.0f;   // Seconds
    float range_ = 5.0f;      // Units

    int32_t level_ = 0;       // 0 = not learned
    int32_t maxLevel_ = 5;

    std::vector<SkillEffect> effects_;
    SkillRequirement requirement_;
};

} // namespace mmorpg
