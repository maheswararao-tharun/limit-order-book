#include <gtest/gtest.h>
#include "lob/book.hpp"
#include "lob/types.hpp"

TEST(BookTestSuite, addOrderInBook) {
    lob::Book orderBook;
    lob::Order sampleOrder {
        .id = 1,
        .side = lob::Side::buy,
        .price = 250,
        .quantity = 2,
        .entryTime = 1000
    };
    orderBook.addOrder(sampleOrder);

    EXPECT_EQ(orderBook.buyTree.size(), 1);           // how many price levels exist
    EXPECT_EQ(orderBook.buyTree[250].orders.size(), 1); // how many orders at price 250
    EXPECT_EQ(orderBook.buyTree[250].orders.front().id, 1); // the id of that one order
}
