#include <gtest/gtest.h>
#include "network/Socket.hpp"
#include "network/Connection.hpp"
#include "proto/messages.pb.h"
#include <boost/asio.hpp>

using namespace mmorpg;
using namespace mmorpg::net;
namespace asio = boost::asio;

class NetworkTest : public ::testing::Test {
protected:
    asio::io_context io;
};

TEST_F(NetworkTest, SocketCreation) {
    Socket socket(io);
    // Socket is created but not connected yet
    EXPECT_FALSE(socket.isValid());  // Not opened until bind/connect
}

TEST_F(NetworkTest, ConnectionIdGeneration) {
    asio::io_context io1, io2;
    Socket socket1(io1);
    Socket socket2(io2);

    // Test that connections get unique UUIDs
    Connection conn1(std::move(socket1), 1);
    Connection conn2(std::move(socket2), 2);

    EXPECT_NE(conn1.getId(), conn2.getId());
    EXPECT_NE(conn1.getUUID(), conn2.getUUID());
}

TEST_F(NetworkTest, ConnectionActorId) {
    Socket socket(io);
    Connection conn(std::move(socket), 1);

    EXPECT_EQ(conn.getActorId(), 0);  // Initially 0

    conn.setActorId(42);
    EXPECT_EQ(conn.getActorId(), 42);
}

TEST_F(NetworkTest, ConnectionDisconnect) {
    Socket socket(io);
    Connection conn(std::move(socket), 1);

    // Initially not connected (socket not established)
    EXPECT_FALSE(conn.isConnected());

    conn.disconnect();
    EXPECT_FALSE(conn.isConnected());
}

TEST_F(NetworkTest, PacketSerialization) {
    proto::Packet packet;
    packet.set_type(static_cast<uint32_t>(proto::MSG_LOGIN_REQUEST));

    proto::LoginRequest loginReq;
    loginReq.set_username("TestUser");
    loginReq.set_password("TestPass");
    packet.set_payload(loginReq.SerializeAsString());

    // Serialize and deserialize
    std::string serialized = packet.SerializeAsString();
    EXPECT_GT(serialized.size(), 0);

    proto::Packet deserialized;
    EXPECT_TRUE(deserialized.ParseFromString(serialized));
    EXPECT_EQ(deserialized.type(), static_cast<uint32_t>(proto::MSG_LOGIN_REQUEST));

    proto::LoginRequest parsedReq;
    EXPECT_TRUE(parsedReq.ParseFromString(deserialized.payload()));
    EXPECT_EQ(parsedReq.username(), "TestUser");
    EXPECT_EQ(parsedReq.password(), "TestPass");
}

TEST_F(NetworkTest, ActorInfoSerialization) {
    proto::ActorInfo info;
    info.set_id(123);
    info.set_name("TestActor");
    info.set_level(5);
    info.set_current_hp(100);
    info.set_max_hp(200);
    info.set_current_mp(50);
    info.set_max_mp(100);

    std::string serialized = info.SerializeAsString();
    EXPECT_GT(serialized.size(), 0);

    proto::ActorInfo deserialized;
    EXPECT_TRUE(deserialized.ParseFromString(serialized));
    EXPECT_EQ(deserialized.id(), 123);
    EXPECT_EQ(deserialized.name(), "TestActor");
    EXPECT_EQ(deserialized.level(), 5);
    EXPECT_EQ(deserialized.current_hp(), 100);
}

TEST_F(NetworkTest, UUIDGeneration) {
    auto uuid1 = mmorpg::generateUUID();
    auto uuid2 = mmorpg::generateUUID();

    // UUIDs should be unique
    EXPECT_NE(uuid1, uuid2);

    // UUIDs should not be nil
    EXPECT_NE(uuid1, mmorpg::nilUUID());
    EXPECT_NE(uuid2, mmorpg::nilUUID());
}
