#include "lob/optimized_book.hpp"
#include <algorithm>

namespace lob {

namespace {
    inline void setBit(std::vector<uint64_t>& bitset, size_t index) {
        bitset[index / 64] |= (1ULL << (index % 64));
    }
    inline void clearBit(std::vector<uint64_t>& bitset, size_t index) {
        bitset[index / 64] &= ~(1ULL << (index % 64));
    }
}

OptimizedBook::OptimizedBook(Price referencePrice, double bandPercentage)
    : buyTree(0), sellTree(0)
{
    minTick = referencePrice - static_cast<Price>(referencePrice * (bandPercentage / 100));
    maxTick = referencePrice + static_cast<Price>(referencePrice * (bandPercentage / 100));
    size_t numLevels = maxTick - minTick + 1;
    buyTree.resize(numLevels);
    sellTree.resize(numLevels);
    buyBitset.assign((numLevels + 63) / 64, 0);
    sellBitset.assign((numLevels + 63) / 64, 0);
    highestBuy = -1;
    lowestSell = -1;
}

AddResult OptimizedBook::addOrder(Order order) {
    if(order.price < minTick || order.price > maxTick) return AddResult::Rejected;
    orderLoc.insert({order.id, std::pair<Side, Price>(order.side, order.price)});

    size_t index = order.price - minTick;
    if(order.side == Side::buy) {
        if(buyTree[index].orders.empty()) setBit(buyBitset, index);
        buyTree[index].orders.push_back(order);
    } else {
        if(sellTree[index].orders.empty()) setBit(sellBitset, index);
        sellTree[index].orders.push_back(order);
    }
    return AddResult::Accepted;
}

bool OptimizedBook::updateHighestBuy() {
    for (size_t w = buyBitset.size(); w-- > 0; ) {
        uint64_t word = buyBitset[w];
        if (word != 0) {
            int bitPos = 63 - __builtin_clzll(word);
            highestBuy = w * 64 + bitPos;
            return true;
        }
    }
    highestBuy = -1;
    return false;
}

bool OptimizedBook::updateLowestSell() {
    for (size_t w = 0; w < sellBitset.size(); w++) {
        uint64_t word = sellBitset[w];
        if (word != 0) {
            int bitPos = __builtin_ctzll(word);
            lowestSell = w * 64 + bitPos;
            return true;
        }
    }
    lowestSell = -1;
    return false;
}

std::optional<Order> OptimizedBook::matchOrder(Order incomingOrder) {
    Quantity tradeQty, incomingOrderSize = incomingOrder.quantity;
    if(incomingOrder.side == Side::buy) {
        if(!updateLowestSell()) return incomingOrder;
        while(incomingOrderSize != 0 && lowestSell != -1) {
            Order& lowestSellOrder = sellTree[lowestSell].orders.front();
            tradeQty = std::min(incomingOrderSize, lowestSellOrder.quantity);
            lowestSellOrder.quantity -= tradeQty;
            incomingOrderSize -= tradeQty;
            if (lowestSellOrder.quantity == 0) {
                sellTree[lowestSell].orders.pop_front();
                if(sellTree[lowestSell].orders.empty()) {
                    clearBit(sellBitset, lowestSell);
                    updateLowestSell();
                }
            }
        }
        if (incomingOrderSize == 0) return std::nullopt;
        incomingOrder.quantity = incomingOrderSize;
        return incomingOrder;
    } else {
        if(!updateHighestBuy()) return incomingOrder;
        while(incomingOrderSize != 0 && highestBuy != -1) {
            Order& highestBuyOrder = buyTree[highestBuy].orders.front();
            tradeQty = std::min(incomingOrderSize, highestBuyOrder.quantity);
            highestBuyOrder.quantity -= tradeQty;
            incomingOrderSize -= tradeQty;
            if (highestBuyOrder.quantity == 0) {
                buyTree[highestBuy].orders.pop_front();
                if(buyTree[highestBuy].orders.empty()) {
                    clearBit(buyBitset, highestBuy);
                    updateHighestBuy();
                }
            }
        }
        if (incomingOrderSize == 0) return std::nullopt;
        incomingOrder.quantity = incomingOrderSize;
        return incomingOrder;
    }
}

void OptimizedBook::cancelOrder(OrderId orderId) {
    if(orderLoc.find(orderId) == orderLoc.end()) return;
    auto [side, price] = orderLoc.find(orderId)->second;
    size_t index = price - minTick;

    if(side == Side::buy) {
        auto& priceLevelIt = buyTree[index];
        auto exactOrder = std::find_if(priceLevelIt.orders.begin(), priceLevelIt.orders.end(),
            [orderId](const Order& o) { return o.id == orderId; });
        if(exactOrder != priceLevelIt.orders.end()) priceLevelIt.orders.erase(exactOrder);
        if(priceLevelIt.orders.empty()) {
            clearBit(buyBitset, index);
            updateHighestBuy();
        }
        orderLoc.erase(orderId);
    } else {
        auto& priceLevelIt = sellTree[index];
        auto exactOrder = std::find_if(priceLevelIt.orders.begin(), priceLevelIt.orders.end(),
            [orderId](const Order& o) { return o.id == orderId; });
        if(exactOrder != priceLevelIt.orders.end()) priceLevelIt.orders.erase(exactOrder);
        if(priceLevelIt.orders.empty()) {
            clearBit(sellBitset, index);
            updateLowestSell();
        }
        orderLoc.erase(orderId);
    }
}

}