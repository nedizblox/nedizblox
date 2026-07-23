#pragma once

#include <boost/asio.hpp>

#include "packets.hpp"

#include <cstdint>
#include <vector>

namespace net::tcpframing {

template <typename SyncStream>
inline bool writeFramed(SyncStream& socket, const void* data, size_t size, boost::system::error_code& ec) {
    packets::TcpMessageHeader header{static_cast<uint32_t>(size)};

    boost::asio::write(socket, boost::asio::buffer(&header, sizeof(header)), ec);
    if (ec)
        return false;

    if (size > 0) {
        boost::asio::write(socket, boost::asio::buffer(data, size), ec);
        if (ec)
            return false;
    }

    return true;
}

constexpr uint32_t kMaxFrameSize = 10 * 1024 * 1024;

template <typename SyncStream>
inline bool readFramed(SyncStream& socket, std::vector<uint8_t>& payload, boost::system::error_code& ec) {
    packets::TcpMessageHeader header{};

    boost::asio::read(socket, boost::asio::buffer(&header, sizeof(header)), ec);
    if (ec)
        return false;

    if (header.size > kMaxFrameSize) {
        ec = boost::asio::error::message_size;
        return false;
    }

    payload.resize(header.size);
    if (header.size > 0) {
        boost::asio::read(socket, boost::asio::buffer(payload.data(), payload.size()), ec);
        if (ec)
            return false;
    }

    return true;
}

} // namespace net::tcpframing