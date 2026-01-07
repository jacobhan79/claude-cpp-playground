#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "combat/CombatSystem.hpp"
#include "core/EventBus.hpp"
#include <iostream>
#include <iomanip>

using namespace mmorpg;

// Event logger - subscribes to events and logs them
class CombatLogger {
public:
    explicit CombatLogger(EventBus& bus, ActorManager& actors)
        : actors_(actors) {
        handlerId_ = bus.subscribe([this](const GameEvent& event) {
            handleEvent(event);
        });
    }

private:
    void handleEvent(const GameEvent& event) {
        std::visit([this](auto&& e) {
            using T = std::decay_t<decltype(e)>;

            if constexpr (std::is_same_v<T, DamageEvent>) {
                auto attacker = actors_.getActor(e.attacker);
                auto target = actors_.getActor(e.target);
                std::cout << "  [DAMAGE] " << (attacker ? attacker->getName() : "???")
                          << " dealt " << e.damage << " "
                          << (e.isPhysical ? "physical" : "magical") << " damage to "
                          << (target ? target->getName() : "???");
                if (e.isCritical) std::cout << " (CRITICAL!)";
                std::cout << std::endl;
            }
            else if constexpr (std::is_same_v<T, DeathEvent>) {
                auto actor = actors_.getActor(e.actor);
                auto killer = actors_.getActor(e.killer);
                std::cout << "  [DEATH] " << (actor ? actor->getName() : "???")
                          << " was killed by "
                          << (killer ? killer->getName() : "???") << std::endl;
            }
            else if constexpr (std::is_same_v<T, SkillUsedEvent>) {
                auto caster = actors_.getActor(e.caster);
                std::cout << "  [SKILL] " << (caster ? caster->getName() : "???")
                          << " used skill #" << e.skill << std::endl;
            }
        }, event);
    }

    ActorManager& actors_;
    EventBus::HandlerId handlerId_;
};

void printStatus(const Actor& actor) {
    const auto& runtime = actor.getRuntimeStats();
    const auto& derived = actor.getDerivedStats();
    std::cout << actor.getName() << ": HP " << runtime.currentHp
              << "/" << derived.maxHp << std::endl;
}

int main() {
    std::cout << "=== MMORPG Combat Demo ===" << std::endl;

    // Create systems
    ActorManager actors;
    EventBus events;
    CombatSystem combat(actors, events);

    // Create event logger
    CombatLogger logger(events, actors);

    // Create actors
    auto warrior = actors.createActor<Actor>("Warrior");
    warrior->setPrimaryStat("strength", 25);
    warrior->setPrimaryStat("vitality", 20);
    warrior->setPrimaryStat("agility", 10);

    auto mage = actors.createActor<Actor>("Mage");
    mage->setPrimaryStat("intelligence", 25);
    mage->setPrimaryStat("wisdom", 20);
    mage->setPrimaryStat("vitality", 8);

    auto rogue = actors.createActor<Actor>("Rogue");
    rogue->setPrimaryStat("agility", 25);
    rogue->setPrimaryStat("luck", 20);
    rogue->setPrimaryStat("strength", 15);

    // Print initial stats
    std::cout << "\n--- Initial Status ---" << std::endl;
    printStatus(*warrior);
    printStatus(*mage);
    printStatus(*rogue);

    // Battle!
    std::cout << "\n--- Round 1 ---" << std::endl;
    std::cout << "Warrior attacks Mage:" << std::endl;
    combat.processAction(BasicAttack{warrior->getId(), mage->getId(), true});
    printStatus(*mage);

    std::cout << "\nMage attacks Warrior (magical):" << std::endl;
    combat.processAction(BasicAttack{mage->getId(), warrior->getId(), false});
    printStatus(*warrior);

    std::cout << "\nRogue attacks Mage:" << std::endl;
    combat.processAction(BasicAttack{rogue->getId(), mage->getId(), true});
    printStatus(*mage);

    // More rounds
    std::cout << "\n--- Round 2 ---" << std::endl;
    std::cout << "Warrior attacks Mage:" << std::endl;
    combat.processAction(BasicAttack{warrior->getId(), mage->getId(), true});
    printStatus(*mage);

    std::cout << "\nRogue attacks Mage:" << std::endl;
    combat.processAction(BasicAttack{rogue->getId(), mage->getId(), true});
    printStatus(*mage);

    // Test skill attack
    std::cout << "\n--- Skill Attack ---" << std::endl;
    std::cout << "Warrior uses skill on Mage:" << std::endl;
    combat.processAction(SkillAttack{warrior->getId(), mage->getId(), 1});
    printStatus(*mage);

    // Final status
    std::cout << "\n--- Final Status ---" << std::endl;
    printStatus(*warrior);
    printStatus(*mage);
    printStatus(*rogue);

    std::cout << "\nLiving actors: " << actors.getLivingActors().size()
              << "/" << actors.getActorCount() << std::endl;

    std::cout << "\n=== Demo Complete ===" << std::endl;
    return 0;
}
