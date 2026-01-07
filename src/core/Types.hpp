#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <functional>

namespace mmorpg {

// UUID-based IDs for distributed systems compatibility
using UUID = boost::uuids::uuid;

// UUID generator (thread-local for thread safety)
inline UUID generateUUID() {
    static thread_local boost::uuids::random_generator gen;
    return gen();
}

// Nil UUID for invalid/null values
inline UUID nilUUID() {
    return boost::uuids::nil_uuid();
}

// Type aliases for clarity and consistency
// Note: ActorId and SkillId remain as uint32_t for network efficiency
// Use UUID for connection and session identifiers
using ActorId = uint32_t;
using SkillId = uint32_t;
using SessionId = UUID;  // Changed to UUID
using ConnectionUUID = UUID;
using Tick = uint64_t;

// Forward declarations
class Actor;
class Character;
class Monster;
class Skill;
class EventBus;

namespace net {
class Connection;
}

// Smart pointer aliases
using ActorPtr = std::shared_ptr<Actor>;
using SkillPtr = std::shared_ptr<Skill>;
using EventBusPtr = std::shared_ptr<EventBus>;

// Weak pointers for avoiding cycles
using ActorWeakPtr = std::weak_ptr<Actor>;
using EventBusWeakPtr = std::weak_ptr<EventBus>;

// Constants
constexpr ActorId INVALID_ACTOR_ID = 0;
constexpr SkillId INVALID_SKILL_ID = 0;
inline const SessionId INVALID_SESSION_ID = boost::uuids::nil_uuid();

} // namespace mmorpg
