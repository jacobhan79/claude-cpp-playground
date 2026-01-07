#include "SkillTree.hpp"
#include <algorithm>

namespace mmorpg {

// SkillTree implementation

void SkillTree::addNode(SkillNode node) {
    maxTier_ = std::max(maxTier_, node.tier);
    nodes_[node.skillId] = std::move(node);
}

const SkillNode* SkillTree::getNode(SkillId id) const {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return nullptr;
    return &it->second;
}

std::vector<SkillId> SkillTree::getAvailableSkills(
    const std::unordered_set<SkillId>& learnedSkills,
    int32_t characterLevel
) const {
    std::vector<SkillId> available;

    for (const auto& [id, node] : nodes_) {
        // Skip already learned skills
        if (learnedSkills.count(id)) continue;

        // Check if all prerequisites are met
        bool prereqsMet = true;
        for (SkillId prereq : node.prerequisites) {
            if (!learnedSkills.count(prereq)) {
                prereqsMet = false;
                break;
            }
        }

        if (prereqsMet) {
            // Check character level requirement from skill database
            const auto* skill = SkillDatabase::instance().getSkill(id);
            if (skill && characterLevel >= skill->getRequirement().requiredCharLevel) {
                available.push_back(id);
            }
        }
    }

    return available;
}

bool SkillTree::canLearn(
    SkillId id,
    const std::unordered_set<SkillId>& learnedSkills,
    const std::unordered_map<SkillId, int32_t>& skillLevels,
    int32_t characterLevel
) const {
    // Check if node exists
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return false;

    const SkillNode& node = it->second;

    // Check prerequisites
    for (SkillId prereq : node.prerequisites) {
        if (!learnedSkills.count(prereq)) return false;
    }

    // Check skill requirement from database
    const auto* skill = SkillDatabase::instance().getSkill(id);
    if (!skill) return false;

    const auto& req = skill->getRequirement();

    // Check character level
    if (characterLevel < req.requiredCharLevel) return false;

    // Check prerequisite skill level
    if (req.prerequisiteSkill != INVALID_SKILL_ID) {
        auto levelIt = skillLevels.find(req.prerequisiteSkill);
        if (levelIt == skillLevels.end() || levelIt->second < req.prerequisiteLevel) {
            return false;
        }
    }

    return true;
}

std::vector<SkillId> SkillTree::getSkillsInTier(int32_t tier) const {
    std::vector<SkillId> result;
    for (const auto& [id, node] : nodes_) {
        if (node.tier == tier) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<SkillId> SkillTree::getAllSkillIds() const {
    std::vector<SkillId> result;
    result.reserve(nodes_.size());
    for (const auto& [id, _] : nodes_) {
        result.push_back(id);
    }
    return result;
}

// SkillDatabase implementation

SkillDatabase& SkillDatabase::instance() {
    static SkillDatabase db;
    return db;
}

void SkillDatabase::registerSkill(Skill skill) {
    skills_.emplace(skill.getId(), std::move(skill));
}

const Skill* SkillDatabase::getSkill(SkillId id) const {
    auto it = skills_.find(id);
    if (it == skills_.end()) return nullptr;
    return &it->second;
}

Skill SkillDatabase::getSkillCopy(SkillId id) const {
    auto it = skills_.find(id);
    if (it == skills_.end()) {
        return Skill(INVALID_SKILL_ID, "Invalid");
    }
    return it->second;
}

bool SkillDatabase::hasSkill(SkillId id) const {
    return skills_.find(id) != skills_.end();
}

std::vector<SkillId> SkillDatabase::getAllSkillIds() const {
    std::vector<SkillId> result;
    result.reserve(skills_.size());
    for (const auto& [id, _] : skills_) {
        result.push_back(id);
    }
    return result;
}

void SkillDatabase::clear() {
    skills_.clear();
}

void SkillDatabase::loadDefaultSkills() {
    clear();

    // Tier 1 - Basic Skills
    registerSkill(
        Skill(1, "Slash")
            .withDescription("A basic sword attack")
            .withType(SkillType::Active)
            .withTargetType(TargetType::SingleEnemy)
            .withManaCost(10)
            .withCooldown(2.0f)
            .withMaxLevel(5)
            .withEffect(DamageEffect{30, 1.0f, true})
    );

    registerSkill(
        Skill(2, "Fireball")
            .withDescription("Launches a ball of fire at the enemy")
            .withType(SkillType::Active)
            .withTargetType(TargetType::SingleEnemy)
            .withManaCost(25)
            .withCooldown(3.0f)
            .withRange(10.0f)
            .withMaxLevel(5)
            .withEffect(DamageEffect{50, 1.2f, false})
    );

    registerSkill(
        Skill(3, "Heal")
            .withDescription("Restores HP to self or ally")
            .withType(SkillType::Active)
            .withTargetType(TargetType::SingleAlly)
            .withManaCost(30)
            .withCooldown(5.0f)
            .withMaxLevel(5)
            .withEffect(HealEffect{60, 0.8f})
    );

    // Tier 2 - Advanced Skills (require tier 1)
    registerSkill(
        Skill(4, "Power Strike")
            .withDescription("A powerful charged attack")
            .withType(SkillType::Active)
            .withTargetType(TargetType::SingleEnemy)
            .withManaCost(25)
            .withCooldown(5.0f)
            .withMaxLevel(5)
            .withRequirement({1, 2, 5})  // Requires Slash level 2, char level 5
            .withEffect(DamageEffect{80, 1.5f, true})
    );

    registerSkill(
        Skill(5, "Flame Wave")
            .withDescription("Sends a wave of fire in front of you")
            .withType(SkillType::Active)
            .withTargetType(TargetType::AreaEnemy)
            .withManaCost(40)
            .withCooldown(6.0f)
            .withRange(8.0f)
            .withMaxLevel(5)
            .withRequirement({2, 2, 5})  // Requires Fireball level 2
            .withEffect(DamageEffect{40, 1.0f, false})
    );

    registerSkill(
        Skill(6, "Regeneration")
            .withDescription("Heals over time")
            .withType(SkillType::Active)
            .withTargetType(TargetType::SingleAlly)
            .withManaCost(35)
            .withCooldown(10.0f)
            .withMaxLevel(5)
            .withRequirement({3, 2, 5})
            .withEffect(HotEffect{20, 10.0f, 1.0f})
    );

    // Tier 3 - Ultimate Skills
    registerSkill(
        Skill(7, "Berserk")
            .withDescription("Greatly increases attack power")
            .withType(SkillType::Active)
            .withTargetType(TargetType::Self)
            .withManaCost(50)
            .withCooldown(30.0f)
            .withMaxLevel(3)
            .withRequirement({4, 3, 10})
            .withEffect(BuffEffect{"strength", 20, 0.5f, 15.0f})
    );

    registerSkill(
        Skill(8, "Meteor")
            .withDescription("Calls down a devastating meteor")
            .withType(SkillType::Active)
            .withTargetType(TargetType::AreaEnemy)
            .withManaCost(100)
            .withCooldown(60.0f)
            .withRange(15.0f)
            .withMaxLevel(3)
            .withRequirement({5, 3, 10})
            .withEffect(DamageEffect{200, 2.0f, false})
    );

    registerSkill(
        Skill(9, "Divine Shield")
            .withDescription("Creates a shield absorbing damage")
            .withType(SkillType::Active)
            .withTargetType(TargetType::Self)
            .withManaCost(60)
            .withCooldown(45.0f)
            .withMaxLevel(3)
            .withRequirement({6, 3, 10})
            .withEffect(ShieldEffect{200, 10.0f, true, true})
    );
}

} // namespace mmorpg
