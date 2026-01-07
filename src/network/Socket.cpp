#include "Socket.hpp"
#include <iostream>

namespace mmorpg::net {

Socket::Socket(asio::io_context& io)
    : socket_(io) {
}

Socket::Socket(tcp::socket socket)
    : socket_(std::move(socket)) {
}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept
    : socket_(std::move(other.socket_))
    , acceptor_(std::move(other.acceptor_)) {
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        socket_ = std::move(other.socket_);
        acceptor_ = std::move(other.acceptor_);
    }
    return *this;
}

void Socket::close() {
    boost::system::error_code ec;
    if (socket_.is_open()) {
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }
    if (acceptor_ && acceptor_->is_open()) {
        acceptor_->close(ec);
    }
}

bool Socket::bind(uint16_t port) {
    try {
        acceptor_ = std::make_unique<tcp::acceptor>(
            socket_.get_executor(),
            tcp::endpoint(tcp::v4(), port)
        );
        return true;
    } catch (const boost::system::system_error& e) {
        std::cerr << "Bind error: " << e.what() << std::endl;
        return false;
    }
}

bool Socket::listen(int backlog) {
    if (!acceptor_) return false;
    try {
        acceptor_->listen(backlog);
        return true;
    } catch (const boost::system::system_error& e) {
        std::cerr << "Listen error: " << e.what() << std::endl;
        return false;
    }
}

std::optional<Socket> Socket::accept() {
    if (!acceptor_ || !acceptor_->is_open()) {
        return std::nullopt;
    }

    boost::system::error_code ec;
    tcp::socket clientSocket(socket_.get_executor());
    acceptor_->accept(clientSocket, ec);

    if (ec) {
        if (ec != asio::error::would_block) {
            // Real error, not just no pending connection
        }
        return std::nullopt;
    }

    return Socket(std::move(clientSocket));
}

bool Socket::connect(const std::string& host, uint16_t port) {
    try {
        tcp::resolver resolver(socket_.get_executor());
        auto endpoints = resolver.resolve(host, std::to_string(port));
        asio::connect(socket_, endpoints);
        return true;
    } catch (const boost::system::system_error& e) {
        std::cerr << "Connect error: " << e.what() << std::endl;
        return false;
    }
}

bool Socket::send(const std::vector<uint8_t>& data) {
    return send(data.data(), data.size());
}

bool Socket::send(const uint8_t* data, size_t size) {
    if (!isValid()) return false;

    try {
        asio::write(socket_, asio::buffer(data, size));
        return true;
    } catch (const boost::system::system_error& e) {
        std::cerr << "Send error: " << e.what() << std::endl;
        return false;
    }
}

std::optional<std::vector<uint8_t>> Socket::receive(size_t maxSize) {
    if (!isValid()) return std::nullopt;

    try {
        std::vector<uint8_t> buffer(maxSize);
        boost::system::error_code ec;
        size_t received = socket_.read_some(asio::buffer(buffer), ec);

        if (ec) {
            return std::nullopt;
        }

        buffer.resize(received);
        return buffer;
    } catch (const boost::system::system_error&) {
        return std::nullopt;
    }
}

int Socket::receiveInto(uint8_t* buffer, size_t maxSize) {
    if (!isValid()) return -1;

    boost::system::error_code ec;
    size_t received = socket_.read_some(asio::buffer(buffer, maxSize), ec);

    if (ec) {
        return -1;
    }

    return static_cast<int>(received);
}

bool Socket::setNonBlocking(bool nonBlocking) {
    if (!isValid()) return false;

    try {
        socket_.non_blocking(nonBlocking);
        return true;
    } catch (const boost::system::system_error&) {
        return false;
    }
}

bool Socket::setReuseAddr(bool reuse) {
    if (!acceptor_) return false;

    try {
        acceptor_->set_option(tcp::acceptor::reuse_address(reuse));
        return true;
    } catch (const boost::system::system_error&) {
        return false;
    }
}

bool Socket::setNoDelay(bool noDelay) {
    if (!isValid()) return false;

    try {
        socket_.set_option(tcp::no_delay(noDelay));
        return true;
    } catch (const boost::system::system_error&) {
        return false;
    }
}

std::string Socket::getPeerAddress() const {
    if (!socket_.is_open()) return "";

    try {
        auto endpoint = socket_.remote_endpoint();
        return endpoint.address().to_string();
    } catch (const boost::system::system_error&) {
        return "";
    }
}

uint16_t Socket::getPeerPort() const {
    if (!socket_.is_open()) return 0;

    try {
        auto endpoint = socket_.remote_endpoint();
        return endpoint.port();
    } catch (const boost::system::system_error&) {
        return 0;
    }
}

} // namespace mmorpg::net
