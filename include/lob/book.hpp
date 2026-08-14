#pragma once
#include "types.hpp"
#include<optional>
#include<map>
#include<algorithm>
#include<unordered_map>

namespace lob {

    // Skeleton for Order Book 

    struct Book {
        std::map<Price, PriceLevel> buyTree;
        std::map<Price, PriceLevel> sellTree;
        std::map<Price, PriceLevel>::reverse_iterator highestBuy; 
        std::map<Price, PriceLevel>::iterator lowestSell;
        std::unordered_map<OrderId, std::pair<Side, Price>> orderLoc;
        void addOrder(Order order);
        void cancelOrder(OrderId orderId);
        std::optional<Order> matchOrder(Order incomingOrder);
        bool updateHighestBuy();
        bool updateLowestSell();
    };

    void Book::addOrder(Order order) {
        Book::orderLoc.insert({order.id, 
            std::pair<Side, Price>(order.side, order.price)});
        
        if(order.side == Side::buy) {
            buyTree[order.price].orders.push_back(order);
        } else {
            sellTree[order.price].orders.push_back(order);
        }
    }

    std::optional<Order> Book::matchOrder(Order incomingOrder) {
        
        Quantity tradeQty, incomingOrderSize = incomingOrder.quantity;

        if(incomingOrder.side == Side::buy) {
            if(Book::sellTree.empty()) {
                return incomingOrder;
            } 
            
            updateLowestSell();
            while(incomingOrderSize != 0 && !sellTree.empty()) {
                Price lowestSellPrice = Book::lowestSell->first;
                Order& lowestSellOrder = Book::lowestSell->second.orders.front();

                tradeQty = std::min(incomingOrderSize, lowestSellOrder.quantity);
                lowestSellOrder.quantity -= tradeQty;
                incomingOrderSize -= tradeQty;

                if (lowestSellOrder.quantity == 0) {
                    // pop the resting order from the deque
                    Book::lowestSell->second.orders.pop_front();

                    // if the price level's deque is now empty, erase it and update the pointer
                    if(Book::lowestSell->second.orders.empty()) {
                        sellTree.erase(lowestSellPrice);
                        updateLowestSell();
                    }
                }
            }

            if (incomingOrderSize == 0) {
                return std::nullopt;
            } else {
                incomingOrder.quantity = incomingOrderSize;
                return incomingOrder;
            }
            
        } else {
            if(Book::buyTree.empty()) {
                return incomingOrder;
            }

            updateHighestBuy();
            while(incomingOrderSize != 0 && !buyTree.empty()) {
                Price highestBuyPrice = Book::highestBuy->first;
                Order& highestBuyOrder = Book::highestBuy->second.orders.front();

                tradeQty = std::min(incomingOrderSize, highestBuyOrder.quantity);
                highestBuyOrder.quantity -= tradeQty;
                incomingOrderSize -= tradeQty;

                if (highestBuyOrder.quantity == 0) {
                    // pop the resting order from the deque
                    Book::highestBuy->second.orders.pop_front();

                    // if the price level's deque is now empty, erase it and update the pointer
                    if(Book::highestBuy->second.orders.empty()) {
                        buyTree.erase(highestBuyPrice);
                        updateHighestBuy();
                    }
                }
            }

            if (incomingOrderSize == 0) {
                return std::nullopt;
            } else {
                incomingOrder.quantity = incomingOrderSize;
                return incomingOrder;
            }
        }
    }

    bool Book::updateLowestSell() {
        if (sellTree.begin() != sellTree.end()) {
            // sellTree.begin() != end() means the tree has at least one entry
            Book::lowestSell = sellTree.begin();
            return true;
        } else {
            // tree is empty
            return false;
        }
    }

    bool Book::updateHighestBuy() {
        if(buyTree.rbegin() != buyTree.rend()) {
            //buyTree.rbegin() != rend() means the tree has at least one entry
            Book::highestBuy = buyTree.rbegin();
            return true;
        } else {
            //tree is empty
            return false;
        }
    }

    void Book::cancelOrder(OrderId orderId) {
        if(orderLoc.find(orderId) == orderLoc.end()) {
            return;
        }

        auto [side, price] = orderLoc.find(orderId)->second;

        if(side == Side::buy) {
            auto priceLevelIt = buyTree.find(price);
            auto exactOrder = std::find_if(
                priceLevelIt->second.orders.begin(), 
                priceLevelIt->second.orders.end(),
                [orderId](const Order& o) { return o.id == orderId; }
            );

            if(exactOrder != priceLevelIt->second.orders.end()) {
                priceLevelIt->second.orders.erase(exactOrder);
            }

            if(priceLevelIt->second.orders.empty()) {
                buyTree.erase(price);
                updateHighestBuy();
            }

            orderLoc.erase(orderId);
        } else {
            auto priceLevelIt = sellTree.find(price);
            auto exactOrder = std::find_if(
                priceLevelIt->second.orders.begin(), 
                priceLevelIt->second.orders.end(),
                [orderId](const Order& o) { return o.id == orderId; }
            );

            if(exactOrder != priceLevelIt->second.orders.end()) {
                priceLevelIt->second.orders.erase(exactOrder);
            }

            if(priceLevelIt->second.orders.empty()) {
                sellTree.erase(price);
                updateLowestSell();
            }

            orderLoc.erase(orderId);
        }
        

    }
}