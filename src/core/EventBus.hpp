#pragma once

#include "Event.hpp"
#include <vector>
#include <functional>
#include <queue>
#include <mutex>

namespace mmorpg {

class EventBus {
public:
    using EventHandler = std::function<void(const GameEvent&)>;
    using HandlerId = size_t;

    EventBus() = default;
    ~EventBus() = default;

    // Non-copyable
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // Subscribe to all events
    HandlerId subscribe(EventHandler handler);

    // Unsubscribe by handler ID
    void unsubscribe(HandlerId id);

    // Publish an event immediately to all subscribers
    void publish(const GameEvent& event);

    // Queue an event for deferred processing
    void queue(const GameEvent& event);

    // Process all queued events
    void processQueue();

    // Get number of subscribers
    size_t getSubscriberCount() const { return handlers_.size(); }

    // Get number of queued events
    size_t getQueueSize() const { return eventQueue_.size(); }

    // Clear all subscribers
    void clearSubscribers();

    // Clear event queue
    void clearQueue();

private:
    struct HandlerEntry {
        HandlerId id;
        EventHandler handler;
        bool active = true;
    };

    std::vector<HandlerEntry> handlers_;
    std::queue<GameEvent> eventQueue_;
    HandlerId nextId_ = 1;

    // For deferred removal during iteration
    bool isPublishing_ = false;
    std::vector<HandlerId> pendingRemovals_;
};

} // namespace mmorpg
