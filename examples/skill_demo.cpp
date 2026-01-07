#include "actors/Character.hpp"
#include "actors/ActorManager.hpp"
#include "skills/SkillTree.hpp"
#include <iostream>
#include <iomanip>

using namespace mmorpg;

void printSkillInfo(const Skill& skill) {
    std::cout << "  [" << skill.getId() << "] " << skill.getName()
              << " - " << skill.getDescription() << std::endl;
    std::cout << "      Mana: " << skill.getManaCost()
              << " | CD: " << skill.getCooldown() << "s"
              << " | Max Level: " << skill.getMaxLevel() << std::endl;

    for (const auto& effect : skill.getEffects()) {
        std::cout << "      Effect: " << getEffectTypeName(effect) << std::endl;
    }
}

void printCharacterSkills(const Character& character) {
    std::cout << "\n=== " << character.getName() << "'s Skills ===" << std::endl;
    std::cout << "Skill Points: " << character.getSkillPoints() << std::endl;
    std::cout << "Learned Skills:" << std::endl;

    for (SkillId id : character.getLearnedSkills()) {
        const auto* skill = SkillDatabase::instance().getSkill(id);
        if (skill) {
            std::cout << "  - " << skill->getName()
                      << " (Level " << character.getSkillLevel(id) << "/"
                      << skill->getMaxLevel() << ")" << std::endl;
        }
    }
}

int main() {
    std::cout << "=== MMORPG Skill Tree Demo ===" << std::endl;

    // Load default skills
    SkillDatabase::instance().loadDefaultSkills();

    // Build skill tree
    SkillTree tree;

    // Tier 1 skills (no prerequisites)
    tree.addNode({1, {}, {4}, 1, 0.0f, 0.0f});      // Slash -> unlocks Power Strike
    tree.addNode({2, {}, {5}, 1, 1.0f, 0.0f});      // Fireball -> unlocks Flame Wave
    tree.addNode({3, {}, {6}, 1, 2.0f, 0.0f});      // Heal -> unlocks Regeneration

    // Tier 2 skills
    tree.addNode({4, {1}, {7}, 2, 0.0f, 1.0f});     // Power Strike
    tree.addNode({5, {2}, {8}, 2, 1.0f, 1.0f});     // Flame Wave
    tree.addNode({6, {3}, {9}, 2, 2.0f, 1.0f});     // Regeneration

    // Tier 3 skills (ultimate)
    tree.addNode({7, {4}, {}, 3, 0.0f, 2.0f});      // Berserk
    tree.addNode({8, {5}, {}, 3, 1.0f, 2.0f});      // Meteor
    tree.addNode({9, {6}, {}, 3, 2.0f, 2.0f});      // Divine Shield

    // Print all available skills
    std::cout << "\n--- Available Skills in Database ---" << std::endl;
    for (SkillId id : SkillDatabase::instance().getAllSkillIds()) {
        const auto* skill = SkillDatabase::instance().getSkill(id);
        if (skill) {
            printSkillInfo(*skill);
            std::cout << std::endl;
        }
    }

    // Create a character
    ActorManager actors;
    auto hero = actors.createActor<Character>("Hero");
    hero->setSkillTree(tree);
    hero->setPrimaryStat("strength", 18);
    hero->setPrimaryStat("intelligence", 15);

    std::cout << "\n--- Character Created ---" << std::endl;
    std::cout << "Name: " << hero->getName() << std::endl;
    std::cout << "Level: " << hero->getLevel() << std::endl;
    std::cout << "Skill Points: " << hero->getSkillPoints() << std::endl;

    // Show available skills
    std::cout << "\n--- Available Skills to Learn ---" << std::endl;
    for (SkillId id : hero->getAvailableSkills()) {
        const auto* skill = SkillDatabase::instance().getSkill(id);
        if (skill) {
            std::cout << "  [" << id << "] " << skill->getName() << std::endl;
        }
    }

    // Learn some skills
    std::cout << "\n--- Learning Skills ---" << std::endl;
    hero->learnSkill(1);  // Slash
    hero->learnSkill(2);  // Fireball
    hero->learnSkill(3);  // Heal

    printCharacterSkills(*hero);

    // Level up to get more skill points
    std::cout << "\n--- Leveling Up ---" << std::endl;
    hero->gainExperience(500);  // Should level up

    // Upgrade a skill
    std::cout << "\n--- Upgrading Skills ---" << std::endl;
    hero->upgradeSkill(1);  // Upgrade Slash to level 2

    // Try to learn tier 2 skill
    std::cout << "\n--- Trying Tier 2 Skills ---" << std::endl;
    auto available = hero->getAvailableSkills();
    std::cout << "Available skills: ";
    for (SkillId id : available) {
        const auto* skill = SkillDatabase::instance().getSkill(id);
        if (skill) std::cout << skill->getName() << " ";
    }
    std::cout << std::endl;

    // Level up more and try again
    hero->gainExperience(2000);  // More levels

    // Now try to learn Power Strike
    std::cout << "\n--- After More Leveling ---" << std::endl;
    available = hero->getAvailableSkills();
    std::cout << "Available skills: ";
    for (SkillId id : available) {
        const auto* skill = SkillDatabase::instance().getSkill(id);
        if (skill) std::cout << skill->getName() << " ";
    }
    std::cout << std::endl;

    hero->learnSkill(4);  // Power Strike

    // Use skills
    std::cout << "\n--- Using Skills ---" << std::endl;
    hero->useSkill(1);  // Use Slash
    hero->useSkill(2);  // Use Fireball

    // Check cooldowns
    std::cout << "\nSkill Cooldowns:" << std::endl;
    std::cout << "  Slash: " << hero->getSkillCooldown(1) << "s" << std::endl;
    std::cout << "  Fireball: " << hero->getSkillCooldown(2) << "s" << std::endl;

    // Final character state
    printCharacterSkills(*hero);

    std::cout << "\n=== Demo Complete ===" << std::endl;
    return 0;
}
