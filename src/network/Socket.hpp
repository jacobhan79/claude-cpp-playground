#pragma once

#include <boost/asio.hpp>
#include <string>
#include <cstdint>
#include <optional>
#include <vector>
#include <memory>

namespace mmorpg::net {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// Boost.Asio based TCP socket wrapper
class Socket {
public:
    // Create socket with existing io_context
    explicit Socket(asio::io_context& io);

    // Create socket from accepted connection
    Socket(tcp::socket socket);

    ~Socket();

    // Non-copyable, movable
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    // Server operations
    bool bind(uint16_t port);
    bool listen(int backlog = 10);
    std::optional<Socket> accept();

    // Client operations
    bool connect(const std::string& host, uint16_t port);

    // Synchronous data transfer
    bool send(const std::vector<uint8_t>& data);
    bool send(const uint8_t* data, size_t size);
    std::optional<std::vector<uint8_t>> receive(size_t maxSize = 65536);
    int receiveInto(uint8_t* buffer, size_t maxSize);

    // Configuration
    bool setNonBlocking(bool nonBlocking);
    bool setReuseAddr(bool reuse);
    bool setNoDelay(bool noDelay);

    // State
    bool isValid() const { return socket_.is_open(); }
    tcp::socket& getAsioSocket() { return socket_; }
    const tcp::socket& getAsioSocket() const { return socket_; }
    void close();

    // Get peer info
    std::string getPeerAddress() const;
    uint16_t getPeerPort() const;

    // Get native handle for compatibility (returns platform-specific handle)
    auto native_handle() { return socket_.native_handle(); }

private:
    tcp::socket socket_;
    std::unique_ptr<tcp::acceptor> acceptor_;
};

} // namespace mmorpg::net
