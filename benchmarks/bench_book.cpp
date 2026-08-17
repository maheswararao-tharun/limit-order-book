#include <benchmark/benchmark.h>
#include "lob/book.hpp"
#include "lob/optimized_book.hpp"

// AddOrder benchmarks periodically reset the book (excluded from timing)
// to bound orderLoc's growth, since --benchmark_min_time forces enough
// iterations that an unbounded hash map would otherwise dominate the
// measurement with rehashing cost unrelated to addOrder's real per-call cost.

static void BM_AddOrder_NaiveBook(benchmark::State& state) {
    lob::Book book;
    lob::OrderId id = 0;
    for (auto _ : state) {
        if (id % 10000 == 0 && id != 0) {
            state.PauseTiming();
            book = lob::Book();
            state.ResumeTiming();
        }
        lob::Price price = 200 + (id % 10);
        lob::Order order{.id = id++, .side = lob::Side::buy, .price = price, .quantity = 10, .entryTime = id};
        book.addOrder(order);
    }
}
BENCHMARK(BM_AddOrder_NaiveBook);

static void BM_AddOrder_OptimizedBook(benchmark::State& state) {
    lob::OptimizedBook book(20000, 5.0);
    lob::OrderId id = 0;
    for (auto _ : state) {
        if (id % 10000 == 0 && id != 0) {
            state.PauseTiming();
            book = lob::OptimizedBook(20000, 5.0);
            state.ResumeTiming();
        }
        lob::Price price = 19500 + (id % 10);
        lob::Order order{.id = id++, .side = lob::Side::buy, .price = price, .quantity = 10, .entryTime = id};
        book.addOrder(order);
    }
}
BENCHMARK(BM_AddOrder_OptimizedBook);

// --- Sparse-book variants: one resting order fully consumed every
// iteration, so the book returns to empty each time. ---

static void BM_MatchOrder_Sparse_NaiveBook(benchmark::State& state) {
    lob::Book book;
    lob::OrderId id = 0;
    for (auto _ : state) {
        state.PauseTiming();
        lob::Price price = 200 + (id % 10);
        book.addOrder({.id = id, .side = lob::Side::sell, .price = price, .quantity = 10, .entryTime = id});
        state.ResumeTiming();
        auto leftover = book.matchOrder({.id = ++id, .side = lob::Side::buy, .price = price, .quantity = 10, .entryTime = id});
        benchmark::DoNotOptimize(leftover);
    }
}
BENCHMARK(BM_MatchOrder_Sparse_NaiveBook);

static void BM_MatchOrder_Sparse_OptimizedBook(benchmark::State& state) {
    lob::OptimizedBook book(20000, 5.0);
    lob::OrderId id = 0;
    for (auto _ : state) {
        state.PauseTiming();
        lob::Price price = 19500 + (id % 10);
        book.addOrder({.id = id, .side = lob::Side::sell, .price = price, .quantity = 10, .entryTime = id});
        state.ResumeTiming();
        auto leftover = book.matchOrder({.id = ++id, .side = lob::Side::buy, .price = price, .quantity = 10, .entryTime = id});
        benchmark::DoNotOptimize(leftover);
    }
}
BENCHMARK(BM_MatchOrder_Sparse_OptimizedBook);

// --- Dense-book variants: ONE book is built once, before the timed loop,
// with 50 resting orders spread across 50 distinct prices. Each iteration
// consumes exactly one resting order (timed), then immediately adds a
// replacement back at the same price (paused) to keep the book's density
// constant across the whole run -- avoiding the cost of repeatedly
// reconstructing the book itself, which would distort the measurement. ---

static constexpr int kDensePopulation = 50;

static void BM_MatchOrder_Dense_NaiveBook(benchmark::State& state) {
    lob::Book book;
    lob::OrderId id = 0;
    for (int j = 0; j < kDensePopulation; j++) {
        book.addOrder({.id = id++, .side = lob::Side::sell, .price = 200 + j, .quantity = 10, .entryTime = id});
    }
    for (auto _ : state) {
        auto leftover = book.matchOrder({.id = id++, .side = lob::Side::buy, .price = 200, .quantity = 10, .entryTime = id});
        benchmark::DoNotOptimize(leftover);

        state.PauseTiming();
        book.addOrder({.id = id++, .side = lob::Side::sell, .price = 200, .quantity = 10, .entryTime = id}); // replace what was consumed
        state.ResumeTiming();
    }
}
BENCHMARK(BM_MatchOrder_Dense_NaiveBook);

static void BM_MatchOrder_Dense_OptimizedBook(benchmark::State& state) {
    lob::OptimizedBook book(20000, 5.0);
    lob::OrderId id = 0;
    for (int j = 0; j < kDensePopulation; j++) {
        book.addOrder({.id = id++, .side = lob::Side::sell, .price = 19500 + j, .quantity = 10, .entryTime = id});
    }
    for (auto _ : state) {
        auto leftover = book.matchOrder({.id = id++, .side = lob::Side::buy, .price = 19500, .quantity = 10, .entryTime = id});
        benchmark::DoNotOptimize(leftover);

        state.PauseTiming();
        book.addOrder({.id = id++, .side = lob::Side::sell, .price = 19500, .quantity = 10, .entryTime = id});
        state.ResumeTiming();
    }
}
BENCHMARK(BM_MatchOrder_Dense_OptimizedBook);

static void BM_CancelOrder_Dense_NaiveBook(benchmark::State& state) {
    lob::Book book;
    lob::OrderId id = 0;
    for (int j = 0; j < kDensePopulation; j++) {
        book.addOrder({.id = id++, .side = lob::Side::buy, .price = 200 + j, .quantity = 10, .entryTime = id});
    }
    for (auto _ : state) {
        state.PauseTiming();
        lob::OrderId newId = id++;
        book.addOrder({.id = newId, .side = lob::Side::buy, .price = 225, .quantity = 10, .entryTime = id}); // middle price
        state.ResumeTiming();

        book.cancelOrder(newId);
    }
}
BENCHMARK(BM_CancelOrder_Dense_NaiveBook);

static void BM_CancelOrder_Dense_OptimizedBook(benchmark::State& state) {
    lob::OptimizedBook book(20000, 5.0);
    lob::OrderId id = 0;
    for (int j = 0; j < kDensePopulation; j++) {
        book.addOrder({.id = id++, .side = lob::Side::buy, .price = 19500 + j, .quantity = 10, .entryTime = id});
    }
    for (auto _ : state) {
        state.PauseTiming();
        lob::OrderId newId = id++;
        book.addOrder({.id = newId, .side = lob::Side::buy, .price = 19525, .quantity = 10, .entryTime = id});
        state.ResumeTiming();

        book.cancelOrder(newId);
    }
}
BENCHMARK(BM_CancelOrder_Dense_OptimizedBook);

BENCHMARK_MAIN();