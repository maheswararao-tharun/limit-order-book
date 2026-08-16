#pragma once

#include "types.hpp"
#include <vector>
#include <optional>
#include <unordered_map>
#include <utility>

namespace lob {

    enum class AddResult { 
        Accepted, 
        Rejected 
    };
    
    struct OptimizedBook {
        std::vector<PriceLevel> buyTree;
        std::vector<PriceLevel> sellTree;
        Price minTick;
        Price maxTick;
        Price highestBuy;   // -1 means "no resting buy orders at all"
        Price lowestSell;   // -1 means "no resting sell orders at all"
        std::unordered_map<OrderId, std::pair<Side, Price>> orderLoc;

        OptimizedBook(Price referencePrice, double bandPercentage);

        AddResult addOrder(Order order);
        void cancelOrder(OrderId orderId);
        std::optional<Order> matchOrder(Order incomingOrder);
        bool updateHighestBuy();
        bool updateLowestSell();
    };

}