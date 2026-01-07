#include "ActorManager.hpp"

namespace mmorpg {

ActorPtr ActorManager::getActor(ActorId id) const {
    auto it = actors_.find(id);
    if (it == actors_.end()) return nullptr;
    return it->second;
}

bool ActorManager::removeActor(ActorId id) {
    return actors_.erase(id) > 0;
}

bool ActorManager::hasActor(ActorId id) const {
    return actors_.find(id) != actors_.end();
}

std::vector<ActorPtr> ActorManager::getAllActors() const {
    std::vector<ActorPtr> result;
    result.reserve(actors_.size());
    for (const auto& [id, actor] : actors_) {
        result.push_back(actor);
    }
    return result;
}

std::vector<ActorPtr> ActorManager::getActorsWhere(std::function<bool(const Actor&)> predicate) const {
    std::vector<ActorPtr> result;
    for (const auto& [id, actor] : actors_) {
        if (predicate(*actor)) {
            result.push_back(actor);
        }
    }
    return result;
}

std::vector<ActorPtr> ActorManager::getLivingActors() const {
    return getActorsWhere([](const Actor& a) { return a.isAlive(); });
}

void ActorManager::updateAll(Tick currentTick) {
    for (auto& [id, actor] : actors_) {
        actor->update(currentTick);
    }
}

void ActorManager::clear() {
    actors_.clear();
}

} // namespace mmorpg
