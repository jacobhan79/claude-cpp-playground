#pragma once

#include "../core/Types.hpp"
#include "Actor.hpp"
#include <boost/container/flat_map.hpp>
#include <vector>
#include <memory>
#include <functional>

namespace mmorpg {

class ActorManager {
public:
    ActorManager() = default;
    ~ActorManager() = default;

    // Non-copyable
    ActorManager(const ActorManager&) = delete;
    ActorManager& operator=(const ActorManager&) = delete;

    // Factory method to create and register actors
    template<typename T, typename... Args>
    std::shared_ptr<T> createActor(Args&&... args) {
        ActorId id = nextActorId_++;
        auto actor = std::make_shared<T>(id, std::forward<Args>(args)...);
        actors_[id] = actor;
        return actor;
    }

    // Get actor by ID
    ActorPtr getActor(ActorId id) const;

    // Get actor with type check
    template<typename T>
    std::shared_ptr<T> getActorAs(ActorId id) const {
        auto it = actors_.find(id);
        if (it == actors_.end()) return nullptr;
        return std::dynamic_pointer_cast<T>(it->second);
    }

    // Remove actor
    bool removeActor(ActorId id);

    // Check if actor exists
    bool hasActor(ActorId id) const;

    // Get all actors
    std::vector<ActorPtr> getAllActors() const;

    // Get actors matching a predicate
    std::vector<ActorPtr> getActorsWhere(std::function<bool(const Actor&)> predicate) const;

    // Get all living actors
    std::vector<ActorPtr> getLivingActors() const;

    // Update all actors
    void updateAll(Tick currentTick);

    // Get count
    size_t getActorCount() const { return actors_.size(); }

    // Clear all actors
    void clear();

    // Set event bus for new actors
    void setEventBus(EventBusPtr bus) { eventBus_ = bus; }

private:
    boost::container::flat_map<ActorId, ActorPtr> actors_;
    ActorId nextActorId_ = 1;  // 0 is reserved for INVALID_ACTOR_ID
    EventBusWeakPtr eventBus_;
};

} // namespace mmorpg
