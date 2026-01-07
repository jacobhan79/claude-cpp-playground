#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include <iostream>
#include <iomanip>

using namespace mmorpg;

void printActorStats(const Actor& actor) {
    std::cout << "\n=== " << actor.getName() << " (ID: " << actor.getId() << ") ===" << std::endl;
    std::cout << "Level: " << actor.getLevel() << std::endl;

    const auto& primary = actor.getPrimaryStats();
    std::cout << "\nPrimary Stats:" << std::endl;
    std::cout << "  STR: " << primary.strength << "  AGI: " << primary.agility << std::endl;
    std::cout << "  INT: " << primary.intelligence << "  VIT: " << primary.vitality << std::endl;
    std::cout << "  WIS: " << primary.wisdom << "  LUK: " << primary.luck << std::endl;

    const auto& derived = actor.getDerivedStats();
    const auto& runtime = actor.getRuntimeStats();
    std::cout << "\nDerived Stats:" << std::endl;
    std::cout << "  HP: " << runtime.currentHp << "/" << derived.maxHp << std::endl;
    std::cout << "  MP: " << runtime.currentMp << "/" << derived.maxMp << std::endl;
    std::cout << "  Physical ATK: " << derived.physicalAttack << std::endl;
    std::cout << "  Magical ATK: " << derived.magicalAttack << std::endl;
    std::cout << "  Physical DEF: " << derived.physicalDefense << std::endl;
    std::cout << "  Magical DEF: " << derived.magicalDefense << std::endl;
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  Crit Chance: " << (derived.criticalChance * 100) << "%" << std::endl;
    std::cout << "  Dodge Chance: " << (derived.dodgeChance * 100) << "%" << std::endl;
}

int main() {
    std::cout << "=== MMORPG Actor Demo ===" << std::endl;

    // Create actor manager
    ActorManager manager;

    // Create some actors
    auto warrior = manager.createActor<Actor>("Warrior");
    warrior->setPrimaryStat("strength", 20);
    warrior->setPrimaryStat("vitality", 18);
    warrior->setPrimaryStat("agility", 12);

    auto mage = manager.createActor<Actor>("Mage");
    mage->setPrimaryStat("intelligence", 22);
    mage->setPrimaryStat("wisdom", 18);
    mage->setPrimaryStat("vitality", 8);

    // Print initial stats
    printActorStats(*warrior);
    printActorStats(*mage);

    // Test combat
    std::cout << "\n\n=== Combat Test ===" << std::endl;

    int damage = 50;
    std::cout << "\nWarrior attacks Mage for " << damage << " damage!" << std::endl;
    int actualDamage = mage->takeDamage(damage);
    std::cout << "Actual damage dealt: " << actualDamage << std::endl;
    std::cout << "Mage HP: " << mage->getRuntimeStats().currentHp
              << "/" << mage->getDerivedStats().maxHp << std::endl;

    // Test healing
    std::cout << "\nMage heals for 30 HP..." << std::endl;
    int healed = mage->heal(30);
    std::cout << "Actual healing: " << healed << std::endl;
    std::cout << "Mage HP: " << mage->getRuntimeStats().currentHp
              << "/" << mage->getDerivedStats().maxHp << std::endl;

    // Test mana usage
    std::cout << "\nMage casts spell (costs 40 MP)..." << std::endl;
    if (mage->useMana(40)) {
        std::cout << "Spell cast successfully!" << std::endl;
    } else {
        std::cout << "Not enough mana!" << std::endl;
    }
    std::cout << "Mage MP: " << mage->getRuntimeStats().currentMp
              << "/" << mage->getDerivedStats().maxMp << std::endl;

    // Test leveling
    std::cout << "\n\n=== Leveling Test ===" << std::endl;
    std::cout << "Warrior gains 500 experience..." << std::endl;
    warrior->gainExperience(500);
    std::cout << "Warrior is now level " << warrior->getLevel() << std::endl;

    // Print final warrior stats
    printActorStats(*warrior);

    // Test actor manager queries
    std::cout << "\n\n=== Actor Manager Test ===" << std::endl;
    std::cout << "Total actors: " << manager.getActorCount() << std::endl;
    std::cout << "Living actors: " << manager.getLivingActors().size() << std::endl;

    // Kill mage and check again
    std::cout << "\nMage takes fatal damage..." << std::endl;
    mage->takeDamage(999);
    std::cout << "Living actors: " << manager.getLivingActors().size() << std::endl;

    std::cout << "\n=== Demo Complete ===" << std::endl;
    return 0;
}
