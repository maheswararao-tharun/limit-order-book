#include "lob/itch_parser.hpp"
#include <cstring>

namespace lob {

    namespace {
        // Manual byte-swap, avoiding platform-specific headers (endian.h on
        // Linux vs OSByteOrder.h on macOS differ). Assumes a little-endian
        // host, true for essentially all current x86 and ARM hardware --
        // this swap converts host-to-network and network-to-host identically,
        // since applying the same swap twice returns the original value.
        uint64_t byteSwap64(uint64_t v) {
            return ((v & 0xFF00000000000000ULL) >> 56) |
                ((v & 0x00FF000000000000ULL) >> 40) |
                ((v & 0x0000FF0000000000ULL) >> 24) |
                ((v & 0x000000FF00000000ULL) >> 8)  |
                ((v & 0x00000000FF000000ULL) << 8)  |
                ((v & 0x0000000000FF0000ULL) << 24) |
                ((v & 0x000000000000FF00ULL) << 40) |
                ((v & 0x00000000000000FFULL) << 56);
        }
        uint32_t byteSwap32(uint32_t v) {
            return ((v & 0xFF000000) >> 24) |
                ((v & 0x00FF0000) >> 8)  |
                ((v & 0x0000FF00) << 8)  |
                ((v & 0x000000FF) << 24);
        }

        void writeBE64(std::vector<uint8_t>& buf, uint64_t v) {
            uint64_t be = byteSwap64(v);
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&be);
            buf.insert(buf.end(), bytes, bytes + 8);
        }
        void writeBE32(std::vector<uint8_t>& buf, uint32_t v) {
            uint32_t be = byteSwap32(v);
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&be);
            buf.insert(buf.end(), bytes, bytes + 4);
        }
        uint64_t readBE64(const uint8_t* data) {
            uint64_t v;
            std::memcpy(&v, data, 8);
            return byteSwap64(v);
        }
        uint32_t readBE32(const uint8_t* data) {
            uint32_t v;
            std::memcpy(&v, data, 4);
            return byteSwap32(v);
        }
    }

    std::vector<uint8_t> serializeAddOrder(const Order& order) {
        std::vector<uint8_t> buf;
        buf.reserve(ITCH_ADD_ORDER_SIZE);

        buf.push_back(ITCH_MSG_ADD_ORDER);
        writeBE64(buf, order.id);
        buf.push_back(order.side == Side::buy ? 0 : 1);
        writeBE64(buf, static_cast<uint64_t>(order.price));
        writeBE32(buf, order.quantity);

        return buf;
    }

    std::optional<Order> parseItchMessage(const uint8_t* data, size_t length) {
        if (length != ITCH_ADD_ORDER_SIZE) return std::nullopt;
        if (data[0] != ITCH_MSG_ADD_ORDER) return std::nullopt;

        OrderId id = readBE64(data + 1);
        uint8_t sideByte = data[9];
        if (sideByte != 0 && sideByte != 1) return std::nullopt;
        Side side = (sideByte == 0) ? Side::buy : Side::sell;
        Price price = static_cast<Price>(readBE64(data + 10));
        Quantity quantity = readBE32(data + 18);

        Order order{};
        order.id = id;
        order.side = side;
        order.price = price;
        order.quantity = quantity;
        order.entryTime = 0;

        return order;
    }

}