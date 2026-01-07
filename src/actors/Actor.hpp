#pragma once

#include "../core/Types.hpp"
#include "Stats.hpp"
#include <string>
#include <memory>

namespace mmorpg {

// Base class for all game entities with stats
class Actor : public std::enable_shared_from_this<Actor> {
public:
    explicit Actor(ActorId id, std::string name);
    virtual ~Actor() = default;

    // Non-copyable, movable
    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;
    Actor(Actor&&) = default;
    Actor& operator=(Actor&&) = default;

    // Accessors
    ActorId getId() const { return id_; }
    const std::string& getName() const { return name_; }
    int32_t getLevel() const { return level_; }
    int64_t getExperience() const { return experience_; }

    // Stats access
    const PrimaryStats& getPrimaryStats() const { return primaryStats_; }
    const DerivedStats& getDerivedStats() const { return derivedStats_; }
    const RuntimeStats& getRuntimeStats() const { return runtimeStats_; }

    // Stat modification
    void setPrimaryStat(const std::string& stat, int32_t value);
    void modifyPrimaryStat(const std::string& stat, int32_t delta);
    void recalculateDerivedStats();

    // Health/Mana operations
    int32_t takeDamage(int32_t amount);  // Returns actual damage taken
    int32_t heal(int32_t amount);         // Returns actual healing done
    bool useMana(int32_t amount);         // Returns true if enough mana
    int32_t restoreMana(int32_t amount);  // Returns actual mana restored

    // State queries
    bool isAlive() const { return runtimeStats_.currentHp > 0; }
    float getHpPercent() const;
    float getMpPercent() const;

    // Level up
    void gainExperience(int64_t exp);
    virtual void onLevelUp();

    // Update (called each game tick)
    virtual void update(Tick currentTick);

    // Event bus injection
    void setEventBus(EventBusPtr bus) { eventBus_ = bus; }

protected:
    ActorId id_;
    std::string name_;
    int32_t level_ = 1;
    int64_t experience_ = 0;

    PrimaryStats primaryStats_;
    DerivedStats derivedStats_;
    RuntimeStats runtimeStats_;

    EventBusWeakPtr eventBus_;

    // Hook for subclasses
    virtual void onDeath();
    void checkLevelUp();
};

} // namespace mmorpg
