#pragma once

#include "../core/Types.hpp"
#include "Skill.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace mmorpg {

// Node in the skill tree representing a skill and its connections
struct SkillNode {
    SkillId skillId;
    std::vector<SkillId> prerequisites;  // Skills required before learning this
    std::vector<SkillId> unlocks;        // Skills this unlocks when learned
    int32_t tier;                         // Tree depth (1 = basic, 5 = ultimate)

    // Position for UI rendering (optional)
    float uiX = 0.0f;
    float uiY = 0.0f;
};

// Skill tree structure - defines skill progression paths
class SkillTree {
public:
    SkillTree() = default;

    // Tree structure management
    void addNode(SkillNode node);
    const SkillNode* getNode(SkillId id) const;

    // Query available skills based on what's already learned
    std::vector<SkillId> getAvailableSkills(
        const std::unordered_set<SkillId>& learnedSkills,
        int32_t characterLevel
    ) const;

    // Check if a specific skill can be learned
    bool canLearn(
        SkillId id,
        const std::unordered_set<SkillId>& learnedSkills,
        const std::unordered_map<SkillId, int32_t>& skillLevels,
        int32_t characterLevel
    ) const;

    // Get all skills in a specific tier
    std::vector<SkillId> getSkillsInTier(int32_t tier) const;

    // Get all skill IDs in the tree
    std::vector<SkillId> getAllSkillIds() const;

    // Get tier count
    int32_t getMaxTier() const { return maxTier_; }

private:
    std::unordered_map<SkillId, SkillNode> nodes_;
    int32_t maxTier_ = 0;
};

// Singleton database of skill definitions
class SkillDatabase {
public:
    static SkillDatabase& instance();

    // Register a skill definition
    void registerSkill(Skill skill);

    // Get skill by ID (const)
    const Skill* getSkill(SkillId id) const;

    // Get skill copy for modification (e.g., leveling up)
    Skill getSkillCopy(SkillId id) const;

    // Check if skill exists
    bool hasSkill(SkillId id) const;

    // Get all skill IDs
    std::vector<SkillId> getAllSkillIds() const;

    // Load default skills (for demo/testing)
    void loadDefaultSkills();

    // Clear all skills
    void clear();

private:
    SkillDatabase() = default;
    SkillDatabase(const SkillDatabase&) = delete;
    SkillDatabase& operator=(const SkillDatabase&) = delete;

    std::unordered_map<SkillId, Skill> skills_;
};

} // namespace mmorpg
