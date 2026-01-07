#include "Actor.hpp"
#include <algorithm>
#include <iostream>

namespace mmorpg {

Actor::Actor(ActorId id, std::string name)
    : id_(id)
    , name_(std::move(name)) {
    recalculateDerivedStats();
    // Initialize runtime stats to max
    runtimeStats_.currentHp = derivedStats_.maxHp;
    runtimeStats_.currentMp = derivedStats_.maxMp;
}

void Actor::setPrimaryStat(const std::string& stat, int32_t value) {
    if (stat == "strength") primaryStats_.strength = value;
    else if (stat == "agility") primaryStats_.agility = value;
    else if (stat == "intelligence") primaryStats_.intelligence = value;
    else if (stat == "vitality") primaryStats_.vitality = value;
    else if (stat == "wisdom") primaryStats_.wisdom = value;
    else if (stat == "luck") primaryStats_.luck = value;

    recalculateDerivedStats();
}

void Actor::modifyPrimaryStat(const std::string& stat, int32_t delta) {
    if (stat == "strength") primaryStats_.strength += delta;
    else if (stat == "agility") primaryStats_.agility += delta;
    else if (stat == "intelligence") primaryStats_.intelligence += delta;
    else if (stat == "vitality") primaryStats_.vitality += delta;
    else if (stat == "wisdom") primaryStats_.wisdom += delta;
    else if (stat == "luck") primaryStats_.luck += delta;

    recalculateDerivedStats();
}

void Actor::recalculateDerivedStats() {
    int32_t oldMaxHp = derivedStats_.maxHp;
    int32_t oldMaxMp = derivedStats_.maxMp;

    derivedStats_ = StatCalculator::calculate(primaryStats_, level_);

    // Adjust current HP/MP proportionally if max changed
    if (oldMaxHp > 0) {
        float hpRatio = static_cast<float>(runtimeStats_.currentHp) / oldMaxHp;
        runtimeStats_.currentHp = static_cast<int32_t>(hpRatio * derivedStats_.maxHp);
    }
    if (oldMaxMp > 0) {
        float mpRatio = static_cast<float>(runtimeStats_.currentMp) / oldMaxMp;
        runtimeStats_.currentMp = static_cast<int32_t>(mpRatio * derivedStats_.maxMp);
    }
}

int32_t Actor::takeDamage(int32_t amount) {
    if (amount <= 0 || !isAlive()) return 0;

    int32_t actualDamage = std::min(amount, runtimeStats_.currentHp);
    runtimeStats_.currentHp -= actualDamage;

    if (runtimeStats_.currentHp <= 0) {
        runtimeStats_.currentHp = 0;
        onDeath();
    }

    return actualDamage;
}

int32_t Actor::heal(int32_t amount) {
    if (amount <= 0 || !isAlive()) return 0;

    int32_t missingHp = derivedStats_.maxHp - runtimeStats_.currentHp;
    int32_t actualHeal = std::min(amount, missingHp);
    runtimeStats_.currentHp += actualHeal;

    return actualHeal;
}

bool Actor::useMana(int32_t amount) {
    if (amount <= 0) return true;
    if (runtimeStats_.currentMp < amount) return false;

    runtimeStats_.currentMp -= amount;
    return true;
}

int32_t Actor::restoreMana(int32_t amount) {
    if (amount <= 0) return 0;

    int32_t missingMp = derivedStats_.maxMp - runtimeStats_.currentMp;
    int32_t actualRestore = std::min(amount, missingMp);
    runtimeStats_.currentMp += actualRestore;

    return actualRestore;
}

float Actor::getHpPercent() const {
    if (derivedStats_.maxHp == 0) return 0.0f;
    return static_cast<float>(runtimeStats_.currentHp) / derivedStats_.maxHp;
}

float Actor::getMpPercent() const {
    if (derivedStats_.maxMp == 0) return 0.0f;
    return static_cast<float>(runtimeStats_.currentMp) / derivedStats_.maxMp;
}

void Actor::gainExperience(int64_t exp) {
    if (exp <= 0) return;

    experience_ += exp;
    checkLevelUp();
}

void Actor::checkLevelUp() {
    while (experience_ >= StatCalculator::experienceForLevel(level_ + 1)) {
        level_++;
        onLevelUp();
    }
}

void Actor::onLevelUp() {
    // Base implementation: recalculate stats and fully heal
    recalculateDerivedStats();
    runtimeStats_.currentHp = derivedStats_.maxHp;
    runtimeStats_.currentMp = derivedStats_.maxMp;

    std::cout << name_ << " leveled up to " << level_ << "!" << std::endl;
}

void Actor::update(Tick /*currentTick*/) {
    // Base implementation: natural HP/MP regen could go here
    // For now, do nothing
}

void Actor::onDeath() {
    std::cout << name_ << " has died!" << std::endl;
    // Subclasses can override for custom death behavior
}

} // namespace mmorpg
