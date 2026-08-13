#pragma once
#include <cstdint>
#include <deque>

namespace lob {

    // Declared all the types for Order Book Engine
    // Book -> struct
    // Limit -> BuyTree & SellTree -> Map(Red-Black Tree)
    // Side -> Buy or Sell -> enum class
    // Order -> Linked List

    using OrderId = std::uint64_t;

    enum class Side : std::uint8_t {
        buy,
        sell
    };

    using Price = std::int64_t;

    using Timestamp = std::uint64_t;

    using Quantity = std::uint32_t;

    struct Order {
        OrderId id;
        Side side;
        Price price;
        Quantity quantity;
        Timestamp entryTime;
    };

    struct PriceLevel {
        std::deque<Order> orders;
    };

}





