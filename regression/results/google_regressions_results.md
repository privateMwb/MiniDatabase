#MiniDBRegression Report

## get_record

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| get_record_hit | 290 ns | 206 ns | -29.1% |
| get_record_miss | 560 ns | 206 ns | -63.2% |

## get_at

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| get_at_hit | 6 ns | 2 ns | -71.1% |
| get_at_miss | 5 ns | 2 ns | -60.3% |

## qe_select

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| qe_select_all | 8.90 ms | 484.88 us | -94.5% |
| qe_select_eq | 66.45 ms | 159.50 us | -99.8% |
| qe_select_sort | 10.13 ms | 2.60 ms | -74.3% |

## fetch

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| fetch_hit | 204 ns | 54 ns | -73.4% |
| fetch_miss | 18.66 us | 5.15 us | -72.4% |

## peek

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| peek_cache | 126 ns | 45 ns | -64.8% |

## getRecord_hit

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| getRecord_hit_small | 115 ns | 72 ns | -37.6% |
| getRecord_hit_large | 58 ns | 22 ns | -62.4% |

## getRecord

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| getRecord_miss | 6 ns | 2 ns | -68.5% |

## db_table

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| db_table_create | 1.90 us | 300 ns | -84.2% |
| db_table_drop | 1.59 us | 312 ns | -80.4% |
| db_table_get | 67 ns | 27 ns | -60.0% |
| db_table_has | 43 ns | 16 ns | -63.5% |

## record

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| record_add | 12.29 us | 3.57 us | -70.9% |
| record_update | 115 ns | 32 ns | -72.0% |
| record_delete | 509 ns | 130 ns | -74.5% |

## compact

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| compact | 32.97 us | 6.28 us | -80.9% |

## field

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| field_set | 178 ns | 31 ns | -82.9% |
| field_get | 194 ns | 33 ns | -82.9% |
| field_getref | 208 ns | 28 ns | -86.4% |
| field_has | 97 ns | 13 ns | -86.7% |
| field_remove | 1.14 us | 119 ns | -89.5% |

## table_record

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| table_record_insert | 14.71 us | 9.52 us | -35.3% |
| table_record_update | 310 ns | 44 ns | -85.6% |
| table_record_delete | 14 ns | 2 ns | -84.9% |

## rebuild

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| rebuild_index | 276.50 us | 17.68 us | -93.6% |

## db

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| db_ctor | 848 ns | 62 ns | -92.7% |

## db_move

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| db_move_empty | 921 ns | 75 ns | -91.9% |
| db_move_populated | 140.06 us | 12.73 us | -90.9% |

## page_ctor

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| page_ctor_default | 932 ns | 47 ns | -95.0% |
| page_ctor_id | 898 ns | 47 ns | -94.8% |

## page_move

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| page_move_empty | 965 ns | 55 ns | -94.3% |
| page_move_full | 23.61 us | 3.59 us | -84.8% |

## record_ctor

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| record_ctor_default | 93 ns | 20 ns | -78.3% |
| record_ctor_id | 116 ns | 20 ns | -82.4% |
| record_ctor_data | 1.96 us | 126 ns | -93.6% |

## record

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| record_copy | 916 ns | 105 ns | -88.5% |
| record_move | 1.71 us | 248 ns | -85.5% |

## ctor_schema

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| ctor_schema_empty | 897 ns | 74 ns | -91.7% |
| ctor_schema_wide | 1.60 us | 226 ns | -85.9% |

## table_move

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| table_move_empty | 913 ns | 80 ns | -91.2% |
| table_move_populated | 311.34 us | 37.58 us | -87.9% |

## rebuild_pool

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| rebuild_pool_below | 1.25 ms | 6.59 us | -99.5% |
| rebuild_pool_at | 228.46 us | 12.48 us | -94.5% |
| rebuild_pool_above | 457.94 us | 38.42 us | -91.6% |
| rebuild_pool_wellabove | 1.85 ms | 147.59 us | -92.0% |

## db_create

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| db_create_10 | 2.06 us | 218 ns | -89.4% |
| db_create_100 | 2.83 us | 421 ns | -85.1% |
| db_create_200 | 3.58 us | 637 ns | -82.2% |

## db_get

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| db_get_10 | 117 ns | 26 ns | -77.6% |
| db_get_100 | 123 ns | 20 ns | -83.7% |
| db_get_200 | 119 ns | 21 ns | -82.5% |

## select_eq_pages

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| select_eq_pages_1 | 17.98 us | 3.18 us | -82.3% |
| select_eq_pages_100 | 5.67 ms | 318.17 us | -94.4% |
| select_eq_pages_1000 | 52.76 ms | 3.55 ms | -93.3% |

## select_sort_pages

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| select_sort_pages_1 | 159.53 us | 29.59 us | -81.4% |
| select_sort_pages_100 | 35.25 ms | 5.70 ms | -83.8% |
| select_sort_pages_1000 | 390.64 ms | 68.73 ms | -82.4% |

## insertRecord_pages

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| insertRecord_pages_1 | 11.34 us | 9.38 us | -17.3% |
| insertRecord_pages_100 | 13.31 us | 8.53 us | -35.9% |
| insertRecord_pages_1000 | 27.03 us | 5.62 us | -79.2% |

## getRecord_pages

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| getRecord_pages_1 | 135 ns | 22 ns | -83.8% |
| getRecord_pages_100 | 110 ns | 22 ns | -80.1% |
| getRecord_pages_1000 | 109 ns | 22 ns | -79.9% |

## all

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| all_save | 5.90 ms | 1.81 ms | -69.2% |
| all_load | 2.95 ms | 280.75 us | -90.5% |
| all_rebuild | 322.95 us | 20.96 us | -93.5% |
| all_export | 5.22 ms | 1.81 ms | -65.3% |

## write

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| write_slot | 14.04 us | 4.34 us | -69.1% |
| write_atomic | 1.15 ms | 397.83 us | -65.3% |

## read

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| read_slot | 8.49 us | 3.65 us | -57.0% |
| read_file | 17.59 us | 5.98 us | -66.0% |

## record_roundtrip

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| record_roundtrip_tree | 5.06 us | 647 ns | -87.2% |
| record_roundtrip_str | 13.33 us | 2.22 us | -83.3% |

## page_roundtrip

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| page_roundtrip_tree | 271.06 us | 57.49 us | -78.8% |
| page_roundtrip_str | 589.00 us | 121.47 us | -79.4% |

## table_roundtrip

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| table_roundtrip_tree | 2.38 ms | 419.92 us | -82.4% |
| table_roundtrip_str | 4.19 ms | 745.49 us | -82.2% |

## table

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| table_export | 3.84 ms | 735.29 us | -80.8% |
| table_import | 3.12 ms | 421.95 us | -86.5% |

## database

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| database_export | 4.67 ms | 848.05 us | -81.9% |
| database_import | 4.05 ms | 432.60 us | -89.3% |

## wal

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| wal_append | 923.11 us | 229.25 us | -75.2% |
| wal_entryAt | 3.67 us | 1.22 us | -66.8% |
| wal_range | 298.41 us | 127.14 us | -57.4% |
| wal_openRecoveryScan | 4.23 ms | 1.33 ms | -68.5% |
| wal_truncateFrom | 623.77 us | 361.73 us | -42.0% |

## Summary

| Result | Count |
|---|---|
| Current faster | 0 (0%) |
| v1.0.0 faster | 91 (100%) |
| Tie | 0 (0%) |
