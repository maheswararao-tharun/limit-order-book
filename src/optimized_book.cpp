#include "lob/optimized_book.hpp"
#include "lob/types.hpp"

lob::OptimizedBook::OptimizedBook(Price referencePrice, double bandPercentage)
    : buyTree(0), sellTree(0)
{
    minTick = referencePrice - static_cast<Price>(referencePrice * (bandPercentage / 100));
    maxTick = referencePrice + static_cast<Price>(referencePrice * (bandPercentage / 100));
    buyTree.resize(maxTick - minTick + 1);
    sellTree.resize(maxTick - minTick + 1);
    highestBuy = -1;
    lowestSell = -1;
}

lob::AddResult lob::OptimizedBook::addOrder(Order order) {
    if(order.price < minTick || order.price > maxTick) {
        return lob::AddResult::Rejected;
    }

    OptimizedBook::orderLoc.insert({order.id, 
        std::pair<Side, Price>(order.side, order.price)});

    if(order.side == Side::buy) {
        buyTree[order.price - minTick].orders.push_back(order);
        return AddResult::Accepted;
    } else {
        sellTree[order.price - minTick].orders.push_back(order);
        return AddResult::Accepted;
    }
}

bool lob::OptimizedBook::updateHighestBuy() {
    for(size_t i = buyTree.size(); i-- > 0; ) {
        if(!buyTree[i].orders.empty()) {
            highestBuy = i;
            return true;
        }
    }

    highestBuy = -1;
    return false;
}

bool lob::OptimizedBook::updateLowestSell() {
    for(size_t i = 0; i < sellTree.size(); i++) {
        if(!sellTree[i].orders.empty()) {
            lowestSell = i;
            return true;
        }
    }

    lowestSell = -1;
    return false;
}

std::optional<lob::Order> lob::OptimizedBook::matchOrder(lob::Order incomingOrder) {
    
    Quantity tradeQty, incomingOrderSize = incomingOrder.quantity;
    
    if(incomingOrder.side == Side::buy) {
        if(!updateLowestSell()) {
            return incomingOrder;
        }

        while(incomingOrderSize != 0 && lowestSell != -1) {
            Price lowestSellPrice = lowestSell;
            Order& lowestSellOrder = sellTree[lowestSell].orders.front();

            tradeQty = std::min(incomingOrderSize, lowestSellOrder.quantity);
            lowestSellOrder.quantity -= tradeQty;
            incomingOrderSize -= tradeQty;

            if (lowestSellOrder.quantity == 0) {
                // pop the resting order from the deque
                sellTree[lowestSell].orders.pop_front();

                // if the price level's deque is now empty, erase it and update the pointer
                if(sellTree[lowestSell].orders.empty()) {
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
        if(!updateHighestBuy()) {
            return incomingOrder;
        }

        while(incomingOrderSize != 0 && highestBuy != -1) {
            Price highestBuyPrice = highestBuy;
            Order& highestBuyOrder = buyTree[highestBuy].orders.front();

            tradeQty = std::min(incomingOrderSize, highestBuyOrder.quantity);
            highestBuyOrder.quantity -= tradeQty;
            incomingOrderSize -= tradeQty;

            if (highestBuyOrder.quantity == 0) {
                // pop the resting order from the deque
                buyTree[highestBuy].orders.pop_front();

                // if the price level's deque is now empty, erase it and update the pointer
                if(buyTree[highestBuy].orders.empty()) {
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