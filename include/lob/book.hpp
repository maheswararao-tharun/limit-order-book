#pragma once
#include "types.hpp"
#include<optional>
#include<map>
#include<algorithm>

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

    void Book::addOrder(Order order) {
        // your code here
        if(order.side == Side::buy) {
            buyTree[order.price].orders.push_back(order);
        } else {
            sellTree[order.price].orders.push_back(order);
        }
    }

    std::optional<Order> Book::matchOrder(Order incomingOrder) {
        // your code here
        
        Quantity tradeQty, restingOrderSize, incomingOrderSize = incomingOrder.quantity;

        if(incomingOrder.side == Side::buy) {
            if(Book::sellTree.empty()) {
                return incomingOrder;
            } else {
                Price lowestSellPrice = Book::lowestSell->first;
                Order lowestSellOrder = Book::lowestSell->second.orders.front();
                tradeQty = std::min(incomingOrder.quantity, lowestSellOrder.quantity);
                restingOrderSize = lowestSellOrder.quantity - tradeQty;
                incomingOrderSize -= tradeQty;
                if(incomingOrderSize > 0) {

                } if(incomingOrderSize == 0) {

                }
            }
            
        } else {
            if(Book::buyTree.empty()) {
                return incomingOrder;
            } else {
                Price highestBuyPrice = Book::highestBuy->first;
                Order highestBuyOrder = Book::highestBuy->second.orders.front();
                tradeQty = std::min(incomingOrder.quantity, highestBuyOrder.quantity);
                restingOrderSize = highestBuyOrder.quantity - tradeQty;
                incomingOrderSize -= tradeQty;
                if(incomingOrderSize > 0) {

                } if(incomingOrderSize == 0) {

                }
            }
        }
    }
}