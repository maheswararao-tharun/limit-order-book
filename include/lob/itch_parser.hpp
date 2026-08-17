#pragma once

#include "types.hpp"
#include <cstdint>
#include <vector>
#include <optional>

namespace lob {

    // A simplified ITCH-style binary "Add Order" message.
    // Fixed-width, big-endian (network byte order), matching the general
    // shape of real exchange binary protocols (e.g. NASDAQ ITCH), without
    // implementing the full real protocol or connecting to a live feed.
    //
    // Layout (22 bytes total):
    //   offset  size  field
    //   0       1     message type ('A' = Add Order)
    //   1       8     order id (big-endian uint64)
    //   9       1     side (0 = buy, 1 = sell)
    //   10      8     price (big-endian int64)
    //   18      4     quantity (big-endian uint32)

    constexpr uint8_t ITCH_MSG_ADD_ORDER = 'A';
    constexpr size_t ITCH_ADD_ORDER_SIZE = 22;

    // Converts an Order into its 22-byte wire representation.
    std::vector<uint8_t> serializeAddOrder(const Order& order);

    // Parses a byte buffer into an Order. Returns std::nullopt if the buffer
    // is the wrong length or has an unrecognized message type -- callers must
    // check the result before using it, rather than assuming success.
    std::optional<Order> parseItchMessage(const uint8_t* data, size_t length);

}