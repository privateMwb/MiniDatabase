#MiniDBRegression Report

## get_record

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| get_record_hit | 290 ns | 290 ns | +0.0% |
| get_record_miss | 560 ns | 560 ns | +0.0% |

## get_at

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| get_at_hit | 6 ns | 6 ns | +0.0% |
| get_at_miss | 5 ns | 5 ns | +0.0% |

## qe_select

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| qe_select_all | 8.90 ms | 8.90 ms | +0.0% |
| qe_select_eq | 66.45 ms | 66.45 ms | +0.0% |
| qe_select_sort | 10.13 ms | 10.13 ms | +0.0% |

## fetch

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| fetch_hit | 204 ns | 204 ns | +0.0% |
| fetch_miss | 18.66 us | 18.66 us | +0.0% |

## peek

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| peek_cache | 126 ns | 126 ns | +0.0% |

## getRecord_hit

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| getRecord_hit_small | 115 ns | 115 ns | +0.0% |
| getRecord_hit_large | 58 ns | 58 ns | +0.0% |

## getRecord

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| getRecord_miss | 6 ns | 6 ns | +0.0% |

## db_table

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| db_table_create | 1.90 us | 1.90 us | +0.0% |
| db_table_drop | 1.59 us | 1.59 us | +0.0% |
| db_table_get | 67 ns | 67 ns | +0.0% |
| db_table_has | 43 ns | 43 ns | +0.0% |

## record

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| record_add | 12.29 us | 12.29 us | +0.0% |
| record_update | 115 ns | 115 ns | +0.0% |
| record_delete | 509 ns | 509 ns | +0.0% |

## compact

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| compact | 32.97 us | 32.97 us | +0.0% |

## field

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| field_set | 178 ns | 178 ns | +0.0% |
| field_get | 194 ns | 194 ns | +0.0% |
| field_getref | 208 ns | 208 ns | +0.0% |
| field_has | 97 ns | 97 ns | +0.0% |
| field_remove | 1.14 us | 1.14 us | +0.0% |

## table_record

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| table_record_insert | 14.71 us | 14.71 us | +0.0% |
| table_record_update | 310 ns | 310 ns | +0.0% |
| table_record_delete | 14 ns | 14 ns | +0.0% |

## rebuild

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| rebuild_index | 276.50 us | 276.50 us | +0.0% |

## db

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| db_ctor | 848 ns | 848 ns | +0.0% |

## db_move

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| db_move_empty | 921 ns | 921 ns | +0.0% |
| db_move_populated | 140.06 us | 140.06 us | +0.0% |

## page_ctor

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| page_ctor_default | 932 ns | 932 ns | +0.0% |
| page_ctor_id | 898 ns | 898 ns | +0.0% |

## page_move

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| page_move_empty | 965 ns | 965 ns | +0.0% |
| page_move_full | 23.61 us | 23.61 us | +0.0% |

## record_ctor

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| record_ctor_default | 93 ns | 93 ns | +0.0% |
| record_ctor_id | 116 ns | 116 ns | +0.0% |
| record_ctor_data | 1.96 us | 1.96 us | +0.0% |

## record

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| record_copy | 916 ns | 916 ns | +0.0% |
| record_move | 1.71 us | 1.71 us | +0.0% |

## ctor_schema

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| ctor_schema_empty | 897 ns | 897 ns | +0.0% |
| ctor_schema_wide | 1.60 us | 1.60 us | +0.0% |

## table_move

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| table_move_empty | 913 ns | 913 ns | +0.0% |
| table_move_populated | 311.34 us | 311.34 us | +0.0% |

## rebuild_pool

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| rebuild_pool_below | 1.25 ms | 1.25 ms | +0.0% |
| rebuild_pool_at | 228.46 us | 228.46 us | +0.0% |
| rebuild_pool_above | 457.94 us | 457.94 us | +0.0% |
| rebuild_pool_wellabove | 1.85 ms | 1.85 ms | +0.0% |

## db_create

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| db_create_10 | 2.06 us | 2.06 us | +0.0% |
| db_create_100 | 2.83 us | 2.83 us | +0.0% |
| db_create_200 | 3.58 us | 3.58 us | +0.0% |

## db_get

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| db_get_10 | 117 ns | 117 ns | +0.0% |
| db_get_100 | 123 ns | 123 ns | +0.0% |
| db_get_200 | 119 ns | 119 ns | +0.0% |

## select_eq_pages

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| select_eq_pages_1 | 17.98 us | 17.98 us | +0.0% |
| select_eq_pages_100 | 5.67 ms | 5.67 ms | +0.0% |
| select_eq_pages_1000 | 52.76 ms | 52.76 ms | +0.0% |

## select_sort_pages

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| select_sort_pages_1 | 159.53 us | 159.53 us | +0.0% |
| select_sort_pages_100 | 35.25 ms | 35.25 ms | +0.0% |
| select_sort_pages_1000 | 390.64 ms | 390.64 ms | +0.0% |

## insertRecord_pages

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| insertRecord_pages_1 | 11.34 us | 11.34 us | +0.0% |
| insertRecord_pages_100 | 13.31 us | 13.31 us | +0.0% |
| insertRecord_pages_1000 | 27.03 us | 27.03 us | +0.0% |

## getRecord_pages

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| getRecord_pages_1 | 135 ns | 135 ns | +0.0% |
| getRecord_pages_100 | 110 ns | 110 ns | +0.0% |
| getRecord_pages_1000 | 109 ns | 109 ns | +0.0% |

## all

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| all_save | 5.90 ms | 5.90 ms | +0.0% |
| all_load | 2.95 ms | 2.95 ms | +0.0% |
| all_rebuild | 322.95 us | 322.95 us | +0.0% |
| all_export | 5.22 ms | 5.22 ms | +0.0% |

## write

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| write_slot | 14.04 us | 14.04 us | +0.0% |
| write_atomic | 1.15 ms | 1.15 ms | +0.0% |

## read

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| read_slot | 8.49 us | 8.49 us | +0.0% |
| read_file | 17.59 us | 17.59 us | +0.0% |

## record_roundtrip

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| record_roundtrip_tree | 5.06 us | 5.06 us | +0.0% |
| record_roundtrip_str | 13.33 us | 13.33 us | +0.0% |

## page_roundtrip

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| page_roundtrip_tree | 271.06 us | 271.06 us | +0.0% |
| page_roundtrip_str | 589.00 us | 589.00 us | +0.0% |

## table_roundtrip

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| table_roundtrip_tree | 2.38 ms | 2.38 ms | +0.0% |
| table_roundtrip_str | 4.19 ms | 4.19 ms | +0.0% |

## table

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| table_export | 3.84 ms | 3.84 ms | +0.0% |
| table_import | 3.12 ms | 3.12 ms | +0.0% |

## database

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| database_export | 4.67 ms | 4.67 ms | +0.0% |
| database_import | 4.05 ms | 4.05 ms | +0.0% |

## wal

| Benchmark | Current | v1.0.0 | Δ |
|---|---|---|---|
| wal_append | 923.11 us | 923.11 us | +0.0% |
| wal_entryAt | 3.67 us | 3.67 us | +0.0% |
| wal_range | 298.41 us | 298.41 us | +0.0% |
| wal_openRecoveryScan | 4.23 ms | 4.23 ms | +0.0% |
| wal_truncateFrom | 623.77 us | 623.77 us | +0.0% |

## Summary

| Result | Count |
|---|---|
| Current faster | 0 (0%) |
| v1.0.0 faster | 0 (0%) |
| Tie | 91 (100%) |
