#include "EventBus.hpp"
#include <algorithm>

namespace mmorpg {

EventBus::HandlerId EventBus::subscribe(EventHandler handler) {
    HandlerId id = nextId_++;
    handlers_.push_back({id, std::move(handler), true});
    return id;
}

void EventBus::unsubscribe(HandlerId id) {
    if (isPublishing_) {
        // Defer removal if we're currently publishing
        pendingRemovals_.push_back(id);
        // Mark as inactive immediately to prevent further calls
        for (auto& entry : handlers_) {
            if (entry.id == id) {
                entry.active = false;
                break;
            }
        }
    } else {
        // Remove immediately
        handlers_.erase(
            std::remove_if(handlers_.begin(), handlers_.end(),
                [id](const HandlerEntry& entry) { return entry.id == id; }),
            handlers_.end()
        );
    }
}

void EventBus::publish(const GameEvent& event) {
    isPublishing_ = true;

    for (const auto& entry : handlers_) {
        if (entry.active) {
            entry.handler(event);
        }
    }

    isPublishing_ = false;

    // Process any deferred removals
    if (!pendingRemovals_.empty()) {
        for (HandlerId id : pendingRemovals_) {
            handlers_.erase(
                std::remove_if(handlers_.begin(), handlers_.end(),
                    [id](const HandlerEntry& entry) { return entry.id == id; }),
                handlers_.end()
            );
        }
        pendingRemovals_.clear();
    }
}

void EventBus::queue(const GameEvent& event) {
    eventQueue_.push(event);
}

void EventBus::processQueue() {
    while (!eventQueue_.empty()) {
        GameEvent event = std::move(eventQueue_.front());
        eventQueue_.pop();
        publish(event);
    }
}

void EventBus::clearSubscribers() {
    handlers_.clear();
    pendingRemovals_.clear();
}

void EventBus::clearQueue() {
    std::queue<GameEvent> empty;
    std::swap(eventQueue_, empty);
}

} // namespace mmorpg
