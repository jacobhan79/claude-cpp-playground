#include "Connection.hpp"
#include <cstring>
#include <iostream>
#include <boost/endian/conversion.hpp>

namespace mmorpg::net {

Connection::Connection(Socket socket, ConnectionId id)
    : socket_(std::move(socket))
    , id_(id)
    , uuid_(mmorpg::generateUUID()) {
    recvBuffer_.reserve(RECV_BUFFER_SIZE);
}

Connection::Connection(Socket socket, ConnectionId id, mmorpg::ConnectionUUID uuid)
    : socket_(std::move(socket))
    , id_(id)
    , uuid_(uuid) {
    recvBuffer_.reserve(RECV_BUFFER_SIZE);
}

bool Connection::readFromSocket() {
    uint8_t buffer[4096];
    int received = socket_.receiveInto(buffer, sizeof(buffer));

    if (received <= 0) {
        // Connection closed or error (including EAGAIN/EWOULDBLOCK)
        if (received == 0) {
            return false;  // Connection closed
        }
        // For non-blocking, no data available is not an error
        return true;
    }

    // Append to receive buffer
    recvBuffer_.insert(recvBuffer_.end(), buffer, buffer + received);
    return true;
}

std::vector<proto::Packet> Connection::getCompletePackets() {
    std::vector<proto::Packet> packets;

    while (recvBuffer_.size() >= 4) {
        // Read packet length (4 bytes, network byte order)
        uint32_t packetLen;
        memcpy(&packetLen, recvBuffer_.data(), 4);
        packetLen = boost::endian::big_to_native(packetLen);

        // Sanity check
        if (packetLen > MAX_PACKET_SIZE) {
            std::cerr << "Packet too large: " << packetLen << std::endl;
            disconnect();
            break;
        }

        // Check if we have the complete packet
        if (recvBuffer_.size() < 4 + packetLen) {
            break;  // Wait for more data
        }

        // Parse the packet
        proto::Packet packet;
        if (packet.ParseFromArray(recvBuffer_.data() + 4, packetLen)) {
            packets.push_back(std::move(packet));
        } else {
            std::cerr << "Failed to parse packet" << std::endl;
        }

        // Remove processed data from buffer
        recvBuffer_.erase(recvBuffer_.begin(), recvBuffer_.begin() + 4 + packetLen);
    }

    return packets;
}

bool Connection::sendPacket(proto::MessageType type, const google::protobuf::Message& message) {
    proto::Packet packet;
    packet.set_type(static_cast<uint32_t>(type));
    packet.set_payload(message.SerializeAsString());
    return sendRawPacket(packet);
}

bool Connection::sendRawPacket(const proto::Packet& packet) {
    std::string data = packet.SerializeAsString();
    uint32_t len = boost::endian::native_to_big(static_cast<uint32_t>(data.size()));

    // Send length prefix
    if (!socket_.send(reinterpret_cast<uint8_t*>(&len), 4)) {
        return false;
    }

    // Send data
    return socket_.send(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

} // namespace mmorpg::net
