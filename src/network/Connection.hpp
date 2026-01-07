#pragma once

#include "Socket.hpp"
#include "../core/Types.hpp"
#include "../proto/messages.pb.h"
#include <vector>
#include <queue>
#include <memory>
#include <cstdint>

namespace mmorpg::net {

// Client connection with message buffering
class Connection {
public:
    using ConnectionId = uint32_t;

    explicit Connection(Socket socket, ConnectionId id);
    Connection(Socket socket, ConnectionId id, mmorpg::ConnectionUUID uuid);

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = default;
    Connection& operator=(Connection&&) = default;

    // Accessors
    ConnectionId getId() const { return id_; }
    const mmorpg::ConnectionUUID& getUUID() const { return uuid_; }
    Socket& getSocket() { return socket_; }
    const Socket& getSocket() const { return socket_; }
    bool isConnected() const { return socket_.isValid() && !disconnected_; }

    // Associated actor (set after login)
    uint32_t getActorId() const { return actorId_; }
    void setActorId(uint32_t id) { actorId_ = id; }

    // Read data from socket into buffer
    // Returns false if connection is closed
    bool readFromSocket();

    // Try to parse complete packets from buffer
    // Returns list of complete packets
    std::vector<proto::Packet> getCompletePackets();

    // Send a packet
    bool sendPacket(proto::MessageType type, const google::protobuf::Message& message);
    bool sendRawPacket(const proto::Packet& packet);

    // Mark as disconnected
    void disconnect() { disconnected_ = true; }

    // Get peer info
    std::string getPeerAddress() const { return socket_.getPeerAddress(); }
    uint16_t getPeerPort() const { return socket_.getPeerPort(); }

private:
    Socket socket_;
    ConnectionId id_;
    mmorpg::ConnectionUUID uuid_;
    uint32_t actorId_ = 0;
    bool disconnected_ = false;

    // Receive buffer
    std::vector<uint8_t> recvBuffer_;
    static constexpr size_t RECV_BUFFER_SIZE = 65536;
    static constexpr size_t MAX_PACKET_SIZE = 1024 * 1024;  // 1MB max

    // Packet format: [4 bytes length][protobuf data]
    bool parsePacket(proto::Packet& packet);
};

using ConnectionPtr = std::shared_ptr<Connection>;

} // namespace mmorpg::net
