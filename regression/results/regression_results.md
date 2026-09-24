#CacheProRegression Report

## Element Access

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| get() hit | 10K | 6 ns | 1 ns | -73.0% |
| get() hit | 100K | 5 ns | 2 ns | -58.9% |
| get() hit | 1M | 5 ns | 2 ns | -65.2% |
| peek() hit | 10K | 4 ns | 1 ns | -67.0% |
| peek() hit | 100K | 5 ns | 1 ns | -82.5% |
| peek() hit | 1M | 4 ns | 1 ns | -70.4% |

## Iteration

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| keys() traversal | 10K | 410 ns | 131 ns | -68.0% |
| keys() traversal | 100K | 387 ns | 136 ns | -65.0% |
| keys() traversal | 1M | 381 ns | 135 ns | -64.6% |

## Search

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| contains() miss | 10K | 3 ns | 2 ns | -15.8% |
| contains() miss | 100K | 3 ns | 2 ns | -36.5% |
| contains() miss | 1M | 3 ns | 2 ns | -30.4% |
| get() miss | 10K | 3 ns | 2 ns | -28.0% |
| get() miss | 100K | 3 ns | 2 ns | -42.9% |
| get() miss | 1M | 3 ns | 2 ns | -39.9% |

## Emplace

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| emplace() insert | 10K | 321 ns | 24 ns | -92.4% |
| emplace() insert | 100K | 305 ns | 27 ns | -91.2% |
| emplace() insert | 1M | 509 ns | 27 ns | -94.8% |

## Erase

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| erase() existing | 10K | 789 ns | 38 ns | -95.2% |
| erase() existing | 100K | 713 ns | 37 ns | -94.9% |
| erase() existing | 1M | 557 ns | 24 ns | -95.7% |

## Insert

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| put() insert | 10K | 467 ns | 19 ns | -95.8% |
| put() insert | 100K | 368 ns | 27 ns | -92.6% |
| put() insert | 1M | 259 ns | 21 ns | -91.8% |

## Pop Clear

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| clear() + refill | 10K | 3.14 us | 632 ns | -79.9% |
| clear() + refill | 100K | 3.14 us | 615 ns | -80.4% |
| clear() + refill | 1M | 3.52 us | 634 ns | -82.0% |

## Push Back

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| put() insert (evicting) | 10K | 121 ns | 44 ns | -63.3% |
| put() insert (evicting) | 100K | 121 ns | 44 ns | -64.0% |
| put() insert (evicting) | 1M | 120 ns | 43 ns | -64.2% |

## Construction

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| construct empty | 10K | 15.64 us | 2.10 us | -86.5% |
| construct empty | 100K | 15.57 us | 2.10 us | -86.5% |
| construct empty | 1M | 15.87 us | 2.09 us | -86.8% |

## Move

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| move-assign | 10K | 64 ns | 6 ns | -90.3% |
| move-assign | 100K | 64 ns | 8 ns | -88.3% |
| move-assign | 1M | 65 ns | 7 ns | -89.7% |
| move-construct | 10K | 7.74 us | 832 ns | -89.2% |
| move-construct | 100K | 7.55 us | 796 ns | -89.5% |
| move-construct | 1M | 7.54 us | 797 ns | -89.4% |

## Reallocation

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| resize() grow | 10K | 631 ns | 35 ns | -94.5% |
| resize() grow | 100K | 484 ns | 43 ns | -91.0% |
| resize() grow | 1M | 323 ns | 39 ns | -87.9% |

## Reserve

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| reserve() | 10K | 13 ns | 1 ns | -93.1% |
| reserve() | 100K | 13 ns | 0 ns | -97.6% |
| reserve() | 1M | 14 ns | 1 ns | -94.8% |

## Shrink To Fit

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| shrink_to_fit() | 10K | 9.28 us | 966 ns | -89.6% |
| shrink_to_fit() | 100K | 8.97 us | 987 ns | -89.0% |
| shrink_to_fit() | 1M | 9.12 us | 960 ns | -89.5% |

## Observer

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| hitCount() | 10K | 1 ns | 0 ns | -76.8% |
| hitCount() | 100K | 2 ns | 0 ns | -84.3% |
| hitCount() | 1M | 1 ns | 0 ns | -62.6% |
| missCount() | 10K | 2 ns | 1 ns | -62.6% |
| missCount() | 100K | 1 ns | 1 ns | -52.8% |
| missCount() | 1M | 1 ns | 1 ns | -59.2% |
| hitRate() | 10K | 14 ns | 2 ns | -89.4% |
| hitRate() | 100K | 14 ns | 2 ns | -89.3% |
| hitRate() | 1M | 15 ns | 1 ns | -90.7% |
| mostRecentKey() | 10K | 3 ns | 0 ns | -90.6% |
| mostRecentKey() | 100K | 3 ns | 1 ns | -76.9% |
| mostRecentKey() | 1M | 3 ns | 0 ns | -90.8% |
| leastRecentKey() | 10K | 3 ns | 0 ns | -90.6% |
| leastRecentKey() | 100K | 3 ns | 0 ns | -90.6% |
| leastRecentKey() | 1M | 3 ns | 1 ns | -81.8% |

## State

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| size() | 10K | 2 ns | 0 ns | -81.4% |
| size() | 100K | 1 ns | 0 ns | -71.5% |
| size() | 1M | 1 ns | 1 ns | -58.1% |
| empty() | 10K | 2 ns | 1 ns | -52.0% |
| empty() | 100K | 2 ns | 1 ns | -61.4% |
| empty() | 1M | 2 ns | 1 ns | -77.2% |
| capacity() | 10K | 2 ns | 0 ns | -84.5% |
| capacity() | 100K | 2 ns | 0 ns | -84.3% |
| capacity() | 1M | 2 ns | 0 ns | -84.7% |

## Summary

| Result | Count |
|---|---|
| Current faster | 0 (0%) |
| v1.0.0 faster | 72 (100%) |
| Tie | 0 (0%) |
