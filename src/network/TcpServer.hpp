#pragma once

#include "Socket.hpp"
#include "Connection.hpp"
#include <boost/asio.hpp>
#include <boost/container/flat_map.hpp>
#include <functional>
#include <vector>
#include <memory>

namespace mmorpg::net {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// Single-threaded TCP server using Boost.Asio
class TcpServer {
public:
    using PacketHandler = std::function<void(ConnectionPtr, const proto::Packet&)>;
    using ConnectHandler = std::function<void(ConnectionPtr)>;
    using DisconnectHandler = std::function<void(ConnectionPtr)>;

    explicit TcpServer(uint16_t port);
    ~TcpServer();

    // Non-copyable
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    // Start/stop
    bool start();
    void stop();
    bool isRunning() const { return running_; }

    // Run one iteration of the event loop
    // Returns number of events processed
    int poll(int timeoutMs = 100);

    // Get io_context for integration with other async operations
    asio::io_context& getIoContext() { return io_; }

    // Set handlers
    void onPacket(PacketHandler handler) { packetHandler_ = std::move(handler); }
    void onConnect(ConnectHandler handler) { connectHandler_ = std::move(handler); }
    void onDisconnect(DisconnectHandler handler) { disconnectHandler_ = std::move(handler); }

    // Send to specific connection
    void send(Connection::ConnectionId connId, proto::MessageType type,
              const google::protobuf::Message& message);

    // Broadcast to all connections
    void broadcast(proto::MessageType type, const google::protobuf::Message& message);

    // Broadcast to all except one
    void broadcastExcept(Connection::ConnectionId exceptId, proto::MessageType type,
                         const google::protobuf::Message& message);

    // Get connection by ID
    ConnectionPtr getConnection(Connection::ConnectionId id);

    // Get connection count
    size_t getConnectionCount() const { return connections_.size(); }

    // Disconnect a connection
    void disconnect(Connection::ConnectionId id);

private:
    void startAccept();
    void handleAccept(const boost::system::error_code& ec, tcp::socket socket);
    void processConnection(ConnectionPtr conn);
    void removeDisconnected();

    uint16_t port_;
    asio::io_context io_;
    tcp::acceptor acceptor_;
    bool running_ = false;

    boost::container::flat_map<Connection::ConnectionId, ConnectionPtr> connections_;
    Connection::ConnectionId nextConnectionId_ = 1;

    PacketHandler packetHandler_;
    ConnectHandler connectHandler_;
    DisconnectHandler disconnectHandler_;
};

} // namespace mmorpg::net
