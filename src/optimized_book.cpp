#include<lob/optimized_book.hpp>

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