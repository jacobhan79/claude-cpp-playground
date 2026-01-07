#include <gtest/gtest.h>
#include "combat/CombatSystem.hpp"
#include "combat/DamageCalculator.hpp"
#include "actors/ActorManager.hpp"
#include "core/EventBus.hpp"

using namespace mmorpg;

class CombatTest : public ::testing::Test {
protected:
    std::shared_ptr<EventBus> eventBus;
    std::unique_ptr<ActorManager> actorManager;
    std::unique_ptr<CombatSystem> combatSystem;

    void SetUp() override {
        eventBus = std::make_shared<EventBus>();
        actorManager = std::make_unique<ActorManager>();
        actorManager->setEventBus(eventBus);
        combatSystem = std::make_unique<CombatSystem>(*actorManager, *eventBus);
    }
};

TEST_F(CombatTest, BasicAttackDealsDamage) {
    auto attacker = actorManager->createActor<Actor>("Attacker");
    auto defender = actorManager->createActor<Actor>("Defender");

    attacker->setPrimaryStat("strength", 30);
    int32_t defenderHpBefore = defender->getRuntimeStats().currentHp;

    BasicAttack attack{attacker->getId(), defender->getId(), true};
    combatSystem->processAction(attack);

    EXPECT_LT(defender->getRuntimeStats().currentHp, defenderHpBefore);
}

TEST_F(CombatTest, CannotAttackDeadTarget) {
    auto attacker = actorManager->createActor<Actor>("Attacker");
    auto defender = actorManager->createActor<Actor>("Defender");

    // Kill defender
    defender->takeDamage(9999);
    ASSERT_FALSE(defender->isAlive());

    BasicAttack attack{attacker->getId(), defender->getId(), true};
    bool canAttack = combatSystem->canPerformAction(attack);

    EXPECT_FALSE(canAttack);
}

TEST_F(CombatTest, DeadAttackerCannotAttack) {
    auto attacker = actorManager->createActor<Actor>("Attacker");
    auto defender = actorManager->createActor<Actor>("Defender");

    // Kill attacker
    attacker->takeDamage(9999);

    BasicAttack attack{attacker->getId(), defender->getId(), true};
    bool canAttack = combatSystem->canPerformAction(attack);

    EXPECT_FALSE(canAttack);
}

TEST_F(CombatTest, DamageEventPublished) {
    auto attacker = actorManager->createActor<Actor>("Attacker");
    auto defender = actorManager->createActor<Actor>("Defender");

    // Set high strength to ensure damage is dealt
    attacker->setPrimaryStat("strength", 50);
    // Set low agility to minimize dodge chance
    defender->setPrimaryStat("agility", 0);

    bool eventReceived = false;
    int attempts = 0;
    const int maxAttempts = 10;  // Allow retries due to random dodge

    eventBus->subscribe([&](const GameEvent& event) {
        if (std::get_if<DamageEvent>(&event)) {
            eventReceived = true;
        }
    });

    // Retry a few times in case of dodge (random)
    while (!eventReceived && attempts < maxAttempts) {
        // Heal defender for each attempt
        defender->heal(1000);
        BasicAttack attack{attacker->getId(), defender->getId(), true};
        combatSystem->processAction(attack);
        attempts++;
    }

    EXPECT_TRUE(eventReceived) << "No damage event after " << attempts << " attempts";
}

TEST_F(CombatTest, DeathEventOnKill) {
    auto attacker = actorManager->createActor<Actor>("Attacker");
    auto defender = actorManager->createActor<Actor>("Defender");

    // Make attacker very strong
    attacker->setPrimaryStat("strength", 100);

    // Weaken defender
    defender->takeDamage(defender->getDerivedStats().maxHp - 1);

    bool deathEventReceived = false;
    eventBus->subscribe([&](const GameEvent& event) {
        if (std::get_if<DeathEvent>(&event)) {
            deathEventReceived = true;
        }
    });

    BasicAttack attack{attacker->getId(), defender->getId(), true};
    combatSystem->processAction(attack);

    EXPECT_TRUE(deathEventReceived);
}

TEST_F(CombatTest, DamageCalculatorMinimumDamage) {
    auto attacker = actorManager->createActor<Actor>("Attacker");
    auto defender = actorManager->createActor<Actor>("Defender");

    // Weak attacker, strong defender
    attacker->setPrimaryStat("strength", 1);
    defender->setPrimaryStat("vitality", 100);

    DamageCalculator calc;
    auto result = calc.calculateBasicAttack(*attacker, *defender, true);

    // Should deal at least 1 damage if not dodged
    if (!result.isDodged) {
        EXPECT_GE(result.finalDamage, 1);
    }
}

TEST_F(CombatTest, PhysicalVsMagicalDamage) {
    auto attacker = actorManager->createActor<Actor>("Attacker");
    auto defender = actorManager->createActor<Actor>("Defender");

    attacker->setPrimaryStat("strength", 30);
    attacker->setPrimaryStat("intelligence", 30);

    DamageCalculator calc;
    auto physResult = calc.calculateBasicAttack(*attacker, *defender, true);
    auto magResult = calc.calculateBasicAttack(*attacker, *defender, false);

    // Both should produce valid results
    EXPECT_GE(physResult.rawDamage, 0);
    EXPECT_GE(magResult.rawDamage, 0);
    EXPECT_TRUE(physResult.isPhysical);
    EXPECT_FALSE(magResult.isPhysical);
}

TEST_F(CombatTest, SkillDamageBonus) {
    auto attacker = actorManager->createActor<Actor>("Attacker");
    auto defender = actorManager->createActor<Actor>("Defender");

    DamageCalculator calc;
    auto basicResult = calc.calculateBasicAttack(*attacker, *defender, true);
    auto skillResult = calc.calculateSkillDamage(*attacker, *defender, 50, true);

    // Skill damage should be higher due to bonus
    if (!basicResult.isDodged && !skillResult.isDodged) {
        EXPECT_GT(skillResult.rawDamage, basicResult.rawDamage);
    }
}

TEST_F(CombatTest, InvalidTargetReturnsEmpty) {
    auto attacker = actorManager->createActor<Actor>("Attacker");

    BasicAttack attack{attacker->getId(), 9999, true};  // Non-existent target
    bool canAttack = combatSystem->canPerformAction(attack);

    EXPECT_FALSE(canAttack);
}
