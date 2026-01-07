#include <gtest/gtest.h>
#include "core/EventBus.hpp"
#include "core/Event.hpp"

using namespace mmorpg;

class EventBusTest : public ::testing::Test {
protected:
    EventBus bus;
};

TEST_F(EventBusTest, SubscribeAndPublish) {
    int callCount = 0;

    auto id = bus.subscribe([&callCount](const GameEvent& event) {
        callCount++;
    });

    EXPECT_EQ(bus.getSubscriberCount(), 1);

    DamageEvent dmg{1, 2, 100, false, true};
    bus.publish(dmg);

    EXPECT_EQ(callCount, 1);
}

TEST_F(EventBusTest, Unsubscribe) {
    int callCount = 0;

    auto id = bus.subscribe([&callCount](const GameEvent& event) {
        callCount++;
    });

    EXPECT_EQ(bus.getSubscriberCount(), 1);

    bus.unsubscribe(id);
    EXPECT_EQ(bus.getSubscriberCount(), 0);

    DamageEvent dmg{1, 2, 100, false, true};
    bus.publish(dmg);

    EXPECT_EQ(callCount, 0);
}

TEST_F(EventBusTest, MultipleSubscribers) {
    int count1 = 0, count2 = 0;

    bus.subscribe([&count1](const GameEvent&) { count1++; });
    bus.subscribe([&count2](const GameEvent&) { count2++; });

    EXPECT_EQ(bus.getSubscriberCount(), 2);

    DamageEvent dmg{1, 2, 100, false, true};
    bus.publish(dmg);

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
}

TEST_F(EventBusTest, QueueAndProcess) {
    int callCount = 0;

    bus.subscribe([&callCount](const GameEvent& event) {
        callCount++;
    });

    DamageEvent dmg{1, 2, 100, false, true};
    bus.queue(dmg);
    bus.queue(dmg);
    bus.queue(dmg);

    EXPECT_EQ(callCount, 0);
    EXPECT_EQ(bus.getQueueSize(), 3);

    bus.processQueue();

    EXPECT_EQ(callCount, 3);
    EXPECT_EQ(bus.getQueueSize(), 0);
}

TEST_F(EventBusTest, ClearSubscribers) {
    bus.subscribe([](const GameEvent&) {});
    bus.subscribe([](const GameEvent&) {});

    EXPECT_EQ(bus.getSubscriberCount(), 2);

    bus.clearSubscribers();

    EXPECT_EQ(bus.getSubscriberCount(), 0);
}

TEST_F(EventBusTest, ClearQueue) {
    bus.subscribe([](const GameEvent&) {});

    DamageEvent dmg{1, 2, 100, false, true};
    bus.queue(dmg);
    bus.queue(dmg);

    EXPECT_EQ(bus.getQueueSize(), 2);

    bus.clearQueue();

    EXPECT_EQ(bus.getQueueSize(), 0);
}

TEST_F(EventBusTest, EventTypeDiscrimination) {
    int damageCount = 0;
    int deathCount = 0;

    bus.subscribe([&damageCount, &deathCount](const GameEvent& event) {
        std::visit([&](auto&& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, DamageEvent>) {
                damageCount++;
            } else if constexpr (std::is_same_v<T, DeathEvent>) {
                deathCount++;
            }
        }, event);
    });

    DamageEvent dmg{1, 2, 100, false, true};
    DeathEvent death{2, 1};

    bus.publish(dmg);
    bus.publish(death);

    EXPECT_EQ(damageCount, 1);
    EXPECT_EQ(deathCount, 1);
}

TEST_F(EventBusTest, GetEventTypeName) {
    DamageEvent dmg{1, 2, 100, false, true};
    DeathEvent death{2, 1};
    HealEvent heal{1, 2, 50};
    LevelUpEvent lvl{1, 5, 6};

    EXPECT_STREQ(getEventTypeName(dmg), "DamageEvent");
    EXPECT_STREQ(getEventTypeName(death), "DeathEvent");
    EXPECT_STREQ(getEventTypeName(heal), "HealEvent");
    EXPECT_STREQ(getEventTypeName(lvl), "LevelUpEvent");
}
