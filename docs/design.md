# Design: Low-Latency Order Book Engine

## Why two implementations?
The naive implementation uses std::map which is simplier and easier to implement and bugs are less likely to hide and can use it as practical reference for the array-based version which is complex than the previous version and with manual tick-offset indexing and bounds handling, which creates more opportunities for subtle mistakes. Because agreement between the two doesn't guarantee correctness, both implementations could share the same bug on some rare edge case. I run many randomized order sequences through both and check that their resulting book state matches at every step, using the simpler version as a practical (not perfect) source of truth to catch bugs in the optimized one.

## Naive implementation (baseline)
- Data structure: std::map<Price, PriceLevel> for each side (buy/sell)
- Why this is the right starting point: 
Because highestBuy always points directly at the current best price, checking what the best bid is takes O(1) and no search needed. That pointer only moves when the current price level's orders are completely gone, at which point it re-points to the new best bid. Similarly, because each price level already holds its orders in a deque, adding a new order there just means calling push_back and no search needed, so it's O(1). When an order arrives at a price that isn't already in the tree, the map has to walk down comparing prices to find the right spot, then create a new node with a fresh deque for it and this costs O(log M), where M is the number of distinct price levels. 
- Known limitation: 
The one weak spot in this structure is canceling by order ID: since the tree is sorted by Price, not by orderId, finding a specific order would cost O(N) in the worst case — so a separate OrderId -> Price hash map is used instead, bringing cancel down to an O(1) lookup followed by a small scan of just that price level's deque.

## Optimized implementation (target)
- Data structure: array indexed by price-tick offset
- What it improves over the naive version, and why: 
The array pre-allocates a slot for every possible price, so unlike the tree, there's no difference between inserting at a new price versus an existing one — both just mean a direct lookup using the price to find the slot, which is O(1). Beyond the complexity difference, the array is also faster in practice because its elements sit contiguous in memory — the CPU's prefetcher pulls the next elements in ahead of time, before the code even asks for it. The tree's nodes, by contrast, are each allocated separately, scattered across memory, so walking the tree means the CPU can't predict the next node — a real, physical delay ('cache miss') that has nothing to do with Big-O at all. 
- Tradeoff/risk: 
The cost of the array is that when the current best price's slot becomes empty, finding the new best price means checking each price one at a time until an occupied one is found — in the worst case this is O(M), which happens when the book is sparse (orders spread far apart) rather than dense (orders clustered close together).

## Correctness strategy
- Unit tests for known cases
- Differential testing: [the argument you just built]

## Performance measurement plan
- What will be measured (median latency, p99, throughput)
- Why these specific numbers, not just "is it fast"
- [To be filled in with real numbers on Day 5]

## Results
[Empty for now — this is where Day 5's actual benchmark table goes]