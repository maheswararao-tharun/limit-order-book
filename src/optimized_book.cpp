#include "lob/optimized_book.hpp"

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

    return false;
}

bool lob::OptimizedBook::updateLowestSell() {
    for(size_t i = 0; i < sellTree.size(); i++) {
        if(!sellTree[i].orders.empty()) {
            lowestSell = i;
            return true;
        }
    }

    return false;
}