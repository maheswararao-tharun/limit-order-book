#include <gtest/gtest.h>
#include "lob/itch_parser.hpp"
#include "lob/book.hpp"

TEST(ItchParserTestSuite, roundTripPreservesAllFields) {
    lob::Order original{101, lob::Side::buy, 25005, 250, 0};
    auto bytes = lob::serializeAddOrder(original);
    auto parsed = lob::parseItchMessage(bytes.data(), bytes.size());

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, original.id);
    EXPECT_EQ(parsed->side, original.side);
    EXPECT_EQ(parsed->price, original.price);
    EXPECT_EQ(parsed->quantity, original.quantity);
}

TEST(ItchParserTestSuite, sellSideRoundTrips) {
    lob::Order sellOrder{202, lob::Side::sell, 19999, 75, 0};
    auto bytes = lob::serializeAddOrder(sellOrder);
    auto parsed = lob::parseItchMessage(bytes.data(), bytes.size());
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->side, lob::Side::sell);
}

TEST(ItchParserTestSuite, wrongLengthRejected) {
    std::vector<uint8_t> tooShort(10, 0);
    auto result = lob::parseItchMessage(tooShort.data(), tooShort.size());
    EXPECT_FALSE(result.has_value());
}

TEST(ItchParserTestSuite, wrongMessageTypeRejected) {
    lob::Order o{1, lob::Side::buy, 100, 10, 0};
    auto bytes = lob::serializeAddOrder(o);
    bytes[0] = 'X';
    auto result = lob::parseItchMessage(bytes.data(), bytes.size());
    EXPECT_FALSE(result.has_value());
}

TEST(ItchParserTestSuite, boundaryValuesRoundTrip) {
    lob::Order bigOrder{18446744073709551615ULL, lob::Side::buy, 9223372036854775807LL, 4294967295u, 0};
    auto bytes = lob::serializeAddOrder(bigOrder);
    auto parsed = lob::parseItchMessage(bytes.data(), bytes.size());
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->id, bigOrder.id);
    EXPECT_EQ(parsed->price, bigOrder.price);
    EXPECT_EQ(parsed->quantity, bigOrder.quantity);
}

TEST(ItchParserTestSuite, integratesWithBook) {
    lob::Book book;
    lob::Order incoming{555, lob::Side::buy, 300, 15, 0};
    auto bytes = lob::serializeAddOrder(incoming);
    auto parsed = lob::parseItchMessage(bytes.data(), bytes.size());
    ASSERT_TRUE(parsed.has_value());
    book.addOrder(*parsed);

    ASSERT_EQ(book.buyTree[300].orders.size(), 1u);
    EXPECT_EQ(book.buyTree[300].orders.front().id, 555u);
}