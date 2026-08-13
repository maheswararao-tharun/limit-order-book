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
                if (restingOrderSize == 0) {
                    // pop the resting order from the deque
                    Book::lowestSell->second.orders.pop_front();

                    // if the price level's deque is now empty, erase it and update the pointer
                    if(Book::lowestSell->second.orders.empty()) {
                        sellTree.erase(lowestSellPrice);
                        updateLowestSell();
                    }

                }

                if (incomingOrderSize == 0) {
                    // fully matched, nothing left to rest — return empty optional
                } else {
                    // still quantity left — loop back and match against the next resting order
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
                if (restingOrderSize == 0) {
                    // pop the resting order from the deque
                    // if the price level's deque is now empty, erase it and update the pointer
                }

                if (incomingOrderSize == 0) {
                    // fully matched, nothing left to rest — return empty optional
                } else {
                    // still quantity left — loop back and match against the next resting order
                }
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
}