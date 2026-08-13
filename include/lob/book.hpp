#pragma once
#include "types.hpp"
#include<optional>
#include<map>

namespace lob {

    // Skeleton for Order Book 

    struct Book {
        std::map<Price, PriceLevel> buyTree;
        std::map<Price, PriceLevel> sellTree;
        std::map<Price, PriceLevel>::reverse_iterator highestBuy; 
        std::map<Price, PriceLevel>::iterator lowestSell;
        void addOrder(Order order);
        void cancelOrder(OrderId orderId);
        std::optional<Order> matchOrder(Order incomingOrder);
        bool updateHighestBuy();
        bool updateLowestSell();
    };
}