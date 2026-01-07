#pragma once

#include "Actor.hpp"
#include "../skills/SkillTree.hpp"
#include <unordered_set>
#include <unordered_map>

namespace mmorpg {

// Player character with skill tree and progression
class Character : public Actor {
public:
    Character(ActorId id, std::string name);

    // Skill tree access
    void setSkillTree(const SkillTree& tree) { skillTree_ = tree; }
    const SkillTree& getSkillTree() const { return skillTree_; }

    // Skill points management
    int32_t getSkillPoints() const { return skillPoints_; }
    void addSkillPoints(int32_t points) { skillPoints_ += points; }

    // Learn a new skill
    bool learnSkill(SkillId skillId);

    // Level up an existing skill
    bool upgradeSkill(SkillId skillId);

    // Check if can learn/upgrade
    bool canLearnSkill(SkillId skillId) const;
    bool canUpgradeSkill(SkillId skillId) const;

    // Get skill level (0 = not learned)
    int32_t getSkillLevel(SkillId skillId) const;

    // Check if skill is learned
    bool hasSkill(SkillId skillId) const;

    // Get all learned skill IDs
    const std::unordered_set<SkillId>& getLearnedSkills() const { return learnedSkills_; }

    // Get available skills to learn
    std::vector<SkillId> getAvailableSkills() const;

    // Override level up to grant skill points
    void onLevelUp() override;

    // Use a skill (checks cooldown, mana, etc.)
    bool useSkill(SkillId skillId);

    // Get remaining cooldown for a skill
    float getSkillCooldown(SkillId skillId) const;

    // Update cooldowns
    void update(Tick currentTick) override;

private:
    SkillTree skillTree_;
    int32_t skillPoints_ = 0;

    std::unordered_set<SkillId> learnedSkills_;
    std::unordered_map<SkillId, int32_t> skillLevels_;

    // Cooldown tracking (skill ID -> last use tick)
    std::unordered_map<SkillId, Tick> skillCooldowns_;
    Tick lastUpdateTick_ = 0;

    static constexpr int32_t SKILL_POINTS_PER_LEVEL = 1;
};

} // namespace mmorpg
