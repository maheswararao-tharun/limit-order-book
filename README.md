# Low-Latency Order Book Engine

A limit order book engine in C++17, built to explore and measure the real
performance tradeoffs between a straightforward, tree-based implementation
and a cache-friendly, array-based one — with every design decision
verified by tests and every performance claim backed by benchmarks, not
assumption.

This project intentionally includes two full implementations of the same
matching engine, so the optimized version always has a simple, trustworthy
reference to be checked against.

## Architecture

Order Book Engine
├── Book (naive) — std::map<Price, PriceLevel> per side
├── OptimizedBook (optimized) — array indexed by price-tick offset,
│ with a bitset for O(1)-amortized
│ best-price scanning
└── ITCH-style parser — byte-level binary message parsing,
feeding directly into either book

### Naive implementation (`Book`)

Price levels are stored in a `std::map<Price, PriceLevel>` per side. This
is the deliberately simple version — sorted automatically by price, easy
to reason about, and used throughout development as the source of truth
to catch bugs in the more complex optimized version.

- Best bid/ask lookup: O(1), via a pointer maintained alongside the map
- Insert at an existing price: O(1)
- Insert at a new price: O(log M), where M is the number of distinct
  price levels
- Cancel by order ID: O(1) lookup via a separate `OrderId → Price` hash
  map, followed by a scan of just that one price level's orders

### Optimized implementation (`OptimizedBook`)

Price levels are stored in a flat array, indexed directly by
`price - minTick`. Every price within a configured band (default: ±5%
around a reference price, matching real-world exchange limit-up/limit-down
bands) has a permanently-reserved slot, whether or not it currently holds
any orders.

- Insert at any price (new or existing): O(1), direct index, no tree walk
- Best bid/ask lookup: accelerated by a **bitset** (one bit per price
  level) plus hardware bit-scan instructions (`__builtin_ctzll`/
  `__builtin_clzll`), finding the next occupied price level in roughly
  O(numLevels / 64) word-checks instead of checking every price
  individually
- Cache-friendly: contiguous memory lets the CPU's prefetcher work ahead,
  unlike the naive tree's scattered node allocations

Full design rationale, including the tradeoffs considered and a real
regression found and fixed during benchmarking, is in
[`docs/design.md`](docs/design.md).

### ITCH-style parser

A fixed-width, big-endian binary parser modeled on the general shape of
real exchange protocols like NASDAQ's ITCH feed. Converts raw 22-byte
messages into the engine's `Order` type, feeding directly into either
book. **This does not connect to a live exchange feed** — it demonstrates
the parsing technique (byte-level decoding, network byte order, malformed-
input rejection) using synthetic messages, since a live feed requires paid
market-data access outside this project's scope.

## Benchmark results

Measured with Google Benchmark, on macOS (Apple Silicon). All four
operations show a consistent improvement for the optimized implementation:

| Operation | Naive | Optimized | Improvement |
|---|---|---|---|
| AddOrder | 13.4 ns | 12.4 ns | ~7.5% faster |
| MatchOrder (sparse book) | 421 ns | 404 ns | ~4% faster |
| MatchOrder (dense book, 50 resting orders) | 425 ns | 408 ns | ~4% faster |
| CancelOrder (dense book) | 424 ns | 414 ns | ~2.4% faster |

**This wasn't the first result.** An earlier version of the optimized
book was measurably *slower* at matching (roughly 2.4x) than the naive
tree, because the array's next-best-price scan cost more than the tree's
O(log M) lookup — exactly the tradeoff predicted in the design doc, more
severe in practice than expected. This was fixed by adding a bitset-
accelerated scan (see above), which restored and slightly exceeded parity
with the naive baseline across every measured operation. Full methodology,
including two benchmark-design bugs found and fixed along the way
(unbounded hash-map growth distorting long-running measurements), is
documented in [`docs/design.md`](docs/design.md).

A profiling pass (macOS `sample`) additionally found that `orderLoc` (the
hash map used for O(1) cancel lookups) was a real bottleneck under
sustained load, due to repeated hash-table rehashing — fixed by
pre-reserving its capacity in both implementations' constructors.

### Platform note

macOS does not expose user-space CPU pinning or frequency-scaling control
the way Linux's `taskset`/`cpupower` do — confirmed directly by Google
Benchmark's own runtime warning during every run on this machine. All
other applications were closed and the machine was on AC power during
measurement, as a partial substitute for the standard methodology.

## Testing

20 tests via GoogleTest, covering both implementations and the parser:
correctness of add/match/cancel, FIFO ordering, partial fills, exhaustion,
cancellation from the middle of a deque, out-of-range rejection, sentinel-
pointer reset, array-integrity preservation, and ITCH round-trip/rejection/
boundary-value cases.

```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

## Benchmarking

```bash
./benchmarks/lob_benchmarks
```

## Project structure

include/lob/ # Public headers: types, Book, OptimizedBook, parser
src/ # Implementations
tests/ # GoogleTest suite
benchmarks/ # Google Benchmark harness
docs/design.md # Full design rationale and results

## Planned enhancements (not yet implemented)

The following were part of the original design scope but are documented
here as planned future work, not implemented in the current codebase:

- **Intrusive linked list** for order storage, replacing `std::deque`,
  giving true O(1) cancellation via direct pointer relinking rather than
  a linear scan within a price level
- **Memory pool allocator** for `Order` objects, avoiding per-order heap
  allocation on the hot path
- **Cache-line alignment** (`alignas(64)`) to prevent false sharing,
  relevant once the book is accessed from multiple threads (see below)
- **Lock-free SPSC queue** feeding orders into the matching engine from a
  separate ingestion thread, with the matching engine itself remaining
  single-threaded — matching the real-world pattern used by most
  production matching engines (a single thread owns sequential
  processing; concurrency lives in ingestion, not in the matching logic
  itself)
- **Extended differential testing** running randomized order sequences
  through both implementations and asserting identical resulting state at
  every step, plus ThreadSanitizer verification once the SPSC queue exists

Deliberately out of scope: kernel bypass (DPDK/Solarflare), live exchange
multicast ingestion, and hardware-level tuning (NUMA, huge pages) — these
require specialized infrastructure and hardware access beyond a personal
project's reasonable scope.

## License

MIT