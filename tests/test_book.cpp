#include <gtest/gtest.h>
#include "lob/book.hpp"
#include "lob/optimized_book.hpp"

// ---------------------------------------------------------------
// Book (naive) tests
// ---------------------------------------------------------------

TEST(BookTestSuite, addOrderInBook) {
    lob::Book orderBook;
    lob::Order sampleOrder {
        .id = 1, .side = lob::Side::buy, .price = 250, .quantity = 2, .entryTime = 1000
    };
    orderBook.addOrder(sampleOrder);

    EXPECT_EQ(orderBook.buyTree.size(), 1);
    EXPECT_EQ(orderBook.buyTree[250].orders.size(), 1);
    EXPECT_EQ(orderBook.buyTree[250].orders.front().id, 1);
}

TEST(BookTestSuite, matchOrderFullMatchNoLeftover) {
    lob::Book orderBook;
    orderBook.addOrder({.id=1, .side=lob::Side::sell, .price=200, .quantity=3, .entryTime=1000});
    orderBook.addOrder({.id=2, .side=lob::Side::sell, .price=200, .quantity=5, .entryTime=1001});

    auto leftover = orderBook.matchOrder({.id=3, .side=lob::Side::buy, .price=200, .quantity=6, .entryTime=1002});

    EXPECT_FALSE(leftover.has_value());
    ASSERT_EQ(orderBook.sellTree[200].orders.size(), 1);
    EXPECT_EQ(orderBook.sellTree[200].orders.front().id, 2);
    EXPECT_EQ(orderBook.sellTree[200].orders.front().quantity, 2); // reference-vs-copy check
}

TEST(BookTestSuite, matchOrderExhaustionReturnsLeftover) {
    lob::Book orderBook;
    orderBook.addOrder({.id=1, .side=lob::Side::sell, .price=200, .quantity=3, .entryTime=1000});

    auto leftover = orderBook.matchOrder({.id=2, .side=lob::Side::buy, .price=200, .quantity=10, .entryTime=1001});

    ASSERT_TRUE(leftover.has_value());
    EXPECT_EQ(leftover->quantity, 7);
    EXPECT_TRUE(orderBook.sellTree.empty());
}

TEST(BookTestSuite, cancelOrderFromMiddleOfDeque) {
    lob::Book orderBook;
    orderBook.addOrder({.id=1, .side=lob::Side::buy, .price=250, .quantity=10, .entryTime=1000});
    orderBook.addOrder({.id=2, .side=lob::Side::buy, .price=250, .quantity=20, .entryTime=1001});
    orderBook.addOrder({.id=3, .side=lob::Side::buy, .price=250, .quantity=30, .entryTime=1002});

    orderBook.cancelOrder(2);

    ASSERT_EQ(orderBook.buyTree[250].orders.size(), 2);
    EXPECT_EQ(orderBook.buyTree[250].orders.front().id, 1);
    EXPECT_EQ(orderBook.buyTree[250].orders.back().id, 3);
}

TEST(BookTestSuite, cancelNonexistentOrderDoesNothing) {
    lob::Book orderBook;
    orderBook.addOrder({.id=1, .side=lob::Side::buy, .price=250, .quantity=10, .entryTime=1000});
    orderBook.cancelOrder(999); // should not crash or affect anything
    EXPECT_EQ(orderBook.buyTree[250].orders.size(), 1);
}

TEST(BookTestSuite, cancelLastOrderRemovesPriceLevel) {
    lob::Book orderBook;
    orderBook.addOrder({.id=1, .side=lob::Side::buy, .price=250, .quantity=10, .entryTime=1000});
    orderBook.cancelOrder(1);
    EXPECT_EQ(orderBook.buyTree.size(), 0);
}

// ---------------------------------------------------------------
// OptimizedBook tests
// ---------------------------------------------------------------

TEST(OptimizedBookTestSuite, constructorSizesArraysCorrectly) {
    lob::OptimizedBook book(20000, 5.0);
    EXPECT_EQ(book.minTick, 19000);
    EXPECT_EQ(book.maxTick, 21000);
    EXPECT_EQ(book.buyTree.size(), 2001u);
    EXPECT_EQ(book.sellTree.size(), 2001u);
    EXPECT_EQ(book.highestBuy, -1);
    EXPECT_EQ(book.lowestSell, -1);
}

TEST(OptimizedBookTestSuite, addOrderAcceptedInRange) {
    lob::OptimizedBook book(20000, 5.0);
    auto result = book.addOrder({.id=1, .side=lob::Side::buy, .price=19500, .quantity=10, .entryTime=0});
    EXPECT_EQ(result, lob::AddResult::Accepted);
    EXPECT_EQ(book.buyTree[500].orders.size(), 1u);
}

TEST(OptimizedBookTestSuite, addOrderRejectedOutOfRange) {
    lob::OptimizedBook book(20000, 5.0);
    auto result = book.addOrder({.id=1, .side=lob::Side::buy, .price=21500, .quantity=10, .entryTime=0});
    EXPECT_EQ(result, lob::AddResult::Rejected);
    EXPECT_EQ(book.orderLoc.count(1), 0u);
}

TEST(OptimizedBookTestSuite, matchOrderFullMatchNoLeftover) {
    lob::OptimizedBook book(20000, 5.0);
    book.addOrder({.id=1, .side=lob::Side::sell, .price=19300, .quantity=3, .entryTime=0});
    book.addOrder({.id=2, .side=lob::Side::sell, .price=19300, .quantity=5, .entryTime=1});

    auto leftover = book.matchOrder({.id=3, .side=lob::Side::buy, .price=19300, .quantity=6, .entryTime=2});

    EXPECT_FALSE(leftover.has_value());
    ASSERT_EQ(book.sellTree[300].orders.size(), 1u);
    EXPECT_EQ(book.sellTree[300].orders.front().quantity, 2u);
}

TEST(OptimizedBookTestSuite, matchOrderExhaustionResetsSentinel) {
    lob::OptimizedBook book(20000, 5.0);
    book.addOrder({.id=1, .side=lob::Side::sell, .price=20700, .quantity=10, .entryTime=0});

    auto leftover = book.matchOrder({.id=2, .side=lob::Side::buy, .price=20700, .quantity=15, .entryTime=1});

    ASSERT_TRUE(leftover.has_value());
    EXPECT_EQ(leftover->quantity, 5u);
    EXPECT_EQ(book.lowestSell, -1); // must be reset, not stale
}

TEST(OptimizedBookTestSuite, matchOrderPreservesArrayIntegrity) {
    lob::OptimizedBook book(20000, 5.0);
    book.addOrder({.id=1, .side=lob::Side::sell, .price=19300, .quantity=5, .entryTime=0});
    book.addOrder({.id=2, .side=lob::Side::sell, .price=20700, .quantity=10, .entryTime=1});

    book.matchOrder({.id=3, .side=lob::Side::buy, .price=19300, .quantity=5, .entryTime=2});

    ASSERT_EQ(book.sellTree.size(), 2001u); // must NOT shrink
    ASSERT_EQ(book.sellTree[1700].orders.size(), 1u);
    EXPECT_EQ(book.sellTree[1700].orders.front().id, 2);
    EXPECT_EQ(book.sellTree[1700].orders.front().price, 20700);
}

TEST(OptimizedBookTestSuite, cancelOrderFromMiddleOfDeque) {
    lob::OptimizedBook book(20000, 5.0);
    book.addOrder({.id=1, .side=lob::Side::buy, .price=19500, .quantity=10, .entryTime=0});
    book.addOrder({.id=2, .side=lob::Side::buy, .price=19500, .quantity=20, .entryTime=1});
    book.addOrder({.id=3, .side=lob::Side::buy, .price=19500, .quantity=30, .entryTime=2});

    book.cancelOrder(2);

    ASSERT_EQ(book.buyTree[500].orders.size(), 2u);
    EXPECT_EQ(book.buyTree[500].orders.front().id, 1);
    EXPECT_EQ(book.buyTree[500].orders.back().id, 3);
}

TEST(OptimizedBookTestSuite, cancelOrderUsesCorrectIndex) {
    lob::OptimizedBook book(20000, 5.0);
    book.addOrder({.id=1, .side=lob::Side::sell, .price=20800, .quantity=5, .entryTime=0}); // untouched control
    book.addOrder({.id=2, .side=lob::Side::buy, .price=19500, .quantity=10, .entryTime=1});

    book.cancelOrder(2);

    EXPECT_EQ(book.buyTree[500].orders.size(), 0u); // correctly cleared at right index
    EXPECT_EQ(book.sellTree[1800].orders.size(), 1u); // untouched, proves indexing didn't corrupt other levels
}