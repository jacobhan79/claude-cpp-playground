#include "TcpServer.hpp"
#include <iostream>
#include <algorithm>

namespace mmorpg::net {

TcpServer::TcpServer(uint16_t port)
    : port_(port)
    , acceptor_(io_) {
}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start() {
    if (running_) return true;

    try {
        tcp::endpoint endpoint(tcp::v4(), port_);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
        acceptor_.non_blocking(true);

        running_ = true;
        startAccept();

        std::cout << "Server listening on port " << port_ << std::endl;
        return true;
    } catch (const boost::system::system_error& e) {
        std::cerr << "Server start error: " << e.what() << std::endl;
        return false;
    }
}

void TcpServer::stop() {
    if (!running_) return;

    running_ = false;
    boost::system::error_code ec;
    acceptor_.close(ec);
    connections_.clear();
    io_.stop();

    std::cout << "Server stopped" << std::endl;
}

void TcpServer::startAccept() {
    acceptor_.async_accept(
        [this](const boost::system::error_code& ec, tcp::socket socket) {
            handleAccept(ec, std::move(socket));
        }
    );
}

void TcpServer::handleAccept(const boost::system::error_code& ec, tcp::socket socket) {
    if (!running_) return;

    if (!ec) {
        socket.non_blocking(true);
        socket.set_option(tcp::no_delay(true));

        auto connId = nextConnectionId_++;
        auto conn = std::make_shared<Connection>(Socket(std::move(socket)), connId);

        std::cout << "New connection #" << connId << " from "
                  << conn->getPeerAddress() << ":" << conn->getPeerPort() << std::endl;

        connections_[connId] = conn;

        if (connectHandler_) {
            connectHandler_(conn);
        }
    }

    // Continue accepting
    startAccept();
}

int TcpServer::poll(int timeoutMs) {
    if (!running_) return 0;

    // Reset io_context if it was stopped
    if (io_.stopped()) {
        io_.restart();
    }

    // Run ready handlers
    io_.poll();

    // Process connections for incoming data
    int eventsProcessed = 0;
    for (auto& [id, conn] : connections_) {
        if (conn->isConnected()) {
            // Non-blocking read
            conn->getSocket().getAsioSocket().non_blocking(true);
            if (conn->readFromSocket()) {
                auto packets = conn->getCompletePackets();
                for (auto& packet : packets) {
                    if (packetHandler_) {
                        packetHandler_(conn, packet);
                    }
                    eventsProcessed++;
                }
            }
        }
    }

    // Remove disconnected clients
    removeDisconnected();

    return eventsProcessed;
}

void TcpServer::removeDisconnected() {
    std::vector<Connection::ConnectionId> toRemove;

    for (auto& [id, conn] : connections_) {
        if (!conn->isConnected()) {
            toRemove.push_back(id);
        }
    }

    for (auto id : toRemove) {
        auto it = connections_.find(id);
        if (it != connections_.end()) {
            std::cout << "Connection #" << id << " disconnected" << std::endl;

            if (disconnectHandler_) {
                disconnectHandler_(it->second);
            }
            connections_.erase(it);
        }
    }
}

void TcpServer::send(Connection::ConnectionId connId, proto::MessageType type,
                     const google::protobuf::Message& message) {
    auto it = connections_.find(connId);
    if (it != connections_.end() && it->second->isConnected()) {
        it->second->sendPacket(type, message);
    }
}

void TcpServer::broadcast(proto::MessageType type, const google::protobuf::Message& message) {
    for (auto& [id, conn] : connections_) {
        if (conn->isConnected()) {
            conn->sendPacket(type, message);
        }
    }
}

void TcpServer::broadcastExcept(Connection::ConnectionId exceptId, proto::MessageType type,
                                const google::protobuf::Message& message) {
    for (auto& [id, conn] : connections_) {
        if (id != exceptId && conn->isConnected()) {
            conn->sendPacket(type, message);
        }
    }
}

ConnectionPtr TcpServer::getConnection(Connection::ConnectionId id) {
    auto it = connections_.find(id);
    if (it == connections_.end()) return nullptr;
    return it->second;
}

void TcpServer::disconnect(Connection::ConnectionId id) {
    auto it = connections_.find(id);
    if (it != connections_.end()) {
        it->second->disconnect();
    }
}

} // namespace mmorpg::net
