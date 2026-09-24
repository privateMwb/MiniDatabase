# FalconHTTPBenchmark Results

## Cache Hit

| Test | Iteration | FalconHTTP |
|---|---|---|
| FileCache::get() Existing Entry | 10K | 1.05 ms |
| FileCache::get() Existing Entry | 100K | 9.97 ms |
| FileCache::get() Existing Entry | 1M | 99.61 ms |

## Dispatch

| Test | Iteration | FalconHTTP |
|---|---|---|
| Router::dispatch() Matching Route | 10K | 26.56 ms |
| Router::dispatch() Matching Route | 100K | 264.93 ms |
| Router::dispatch() Matching Route | 1M | 3.56 s |
| Router::dispatch() No Matching Route | 10K | 14.28 ms |
| Router::dispatch() No Matching Route | 100K | 168.16 ms |
| Router::dispatch() No Matching Route | 1M | 1.61 s |

## Match Stream

| Test | Iteration | FalconHTTP |
|---|---|---|
| Router::matchStream() Matching Route | 10K | 31.45 ms |
| Router::matchStream() Matching Route | 100K | 349.49 ms |
| Router::matchStream() Matching Route | 1M | 2.19 s |
| Router::matchStream() No Matching Route | 10K | 11.68 ms |
| Router::matchStream() No Matching Route | 100K | 132.40 ms |
| Router::matchStream() No Matching Route | 1M | 1.20 s |

## Path Match

| Test | Iteration | FalconHTTP |
|---|---|---|
| PathMatcher::match() Two Params | 10K | 7.82 ms |
| PathMatcher::match() Two Params | 100K | 100.25 ms |
| PathMatcher::match() Two Params | 1M | 1.07 s |

## Body

| Test | Iteration | FalconHTTP |
|---|---|---|
| Parse() 64 KiB Body | 10K | 115.24 ms |
| Parse() 64 KiB Body | 100K | 1.13 s |
| Parse() 64 KiB Body | 1M | 32.22 s |
| Serialize() 64 KiB Body | 10K | 305.14 ms |
| Serialize() 64 KiB Body | 100K | 2.62 s |
| Serialize() 64 KiB Body | 1M | 25.10 s |

## Cache Put

| Test | Iteration | FalconHTTP |
|---|---|---|
| FileCache::put() Fresh Key | 10K | 10.05 ms |
| FileCache::put() Fresh Key | 100K | 84.60 ms |
| FileCache::put() Fresh Key | 1M | 831.00 ms |

## Chain Overhead

| Test | Iteration | FalconHTTP |
|---|---|---|
| 3-middleware Chain + Handler | 10K | 19.65 ms |
| 3-middleware Chain + Handler | 100K | 189.73 ms |
| 3-middleware Chain + Handler | 1M | 1.95 s |

## Cors Overhead

| Test | Iteration | FalconHTTP |
|---|---|---|
| Cors::operator() Non-preflight | 10K | 78.91 ms |
| Cors::operator() Non-preflight | 100K | 731.34 ms |
| Cors::operator() Non-preflight | 1M | 7.24 s |

## Header

| Test | Iteration | FalconHTTP |
|---|---|---|
| Parse() 50 Headers | 10K | 492.14 ms |
| Parse() 50 Headers | 100K | 5.22 s |
| Parse() 50 Headers | 1M | 49.65 s |
| Serialize() 50 Headers | 10K | 118.85 ms |
| Serialize() 50 Headers | 100K | 1.23 s |
| Serialize() 50 Headers | 1M | 12.39 s |

## Request Line

| Test | Iteration | FalconHTTP |
|---|---|---|
| Parse() Minimal Request Line | 10K | 38.62 ms |
| Parse() Minimal Request Line | 100K | 370.28 ms |
| Parse() Minimal Request Line | 1M | 3.78 s |

## Sse Send

| Test | Iteration | FalconHTTP |
|---|---|---|
| SseConnection::send() Framing + Write | 10K | 124.28 ms |
| SseConnection::send() Framing + Write | 100K | 4.83 s |
| SseConnection::send() Framing + Write | 1M | 19.37 s |

## Connection Move

| Test | Iteration | FalconHTTP |
|---|---|---|
| Connection Create + Move Construction | 10K | 394.66 ms |
| Connection Create + Move Construction | 100K | 4.28 s |
| Connection Create + Move Construction | 1M | 97.26 s |

## Server Construction

| Test | Iteration | FalconHTTP |
|---|---|---|
| Server Construction, 4 Threads | 10K | 3.45 s |
| Server Construction, 4 Threads | 100K | 22.15 s |
| Server Construction, 4 Threads | 1M | 302.07 s |

## Socket Construction

| Test | Iteration | FalconHTTP |
|---|---|---|
| Socket::createTcp() + Close() | 10K | 1.02 s |
| Socket::createTcp() + Close() | 100K | 10.20 s |
| Socket::createTcp() + Close() | 1M | 114.00 s |

## Header Count Growth

| Test | Iteration | FalconHTTP |
|---|---|---|
| Parse() 5 Headers | 10K | 59.14 ms |
| Parse() 5 Headers | 100K | 597.55 ms |
| Parse() 5 Headers | 1M | 6.20 s |
| Parse() 25 Headers | 10K | 245.72 ms |
| Parse() 25 Headers | 100K | 2.45 s |
| Parse() 25 Headers | 1M | 25.58 s |
| Parse() 100 Headers | 10K | 925.84 ms |
| Parse() 100 Headers | 100K | 9.92 s |
| Parse() 100 Headers | 1M | 77.55 s |
| Parse() 500 Headers | 10K | 2.34 s |
| Parse() 500 Headers | 100K | 24.58 s |
| Parse() 500 Headers | 1M | 462.98 s |

## Middleware Chain Growth

| Test | Iteration | FalconHTTP |
|---|---|---|
| Chain Length 1 | 10K | 18.03 ms |
| Chain Length 1 | 100K | 183.07 ms |
| Chain Length 1 | 1M | 1.78 s |
| Chain Length 5 | 10K | 19.01 ms |
| Chain Length 5 | 100K | 189.05 ms |
| Chain Length 5 | 1M | 1.90 s |
| Chain Length 20 | 10K | 24.56 ms |
| Chain Length 20 | 100K | 244.78 ms |
| Chain Length 20 | 1M | 2.61 s |
| Chain Length 50 | 10K | 53.05 ms |
| Chain Length 50 | 100K | 418.25 ms |
| Chain Length 50 | 1M | 3.55 s |

## Route Table Growth

| Test | Iteration | FalconHTTP |
|---|---|---|
| Dispatch() Last Of 10 Routes | 10K | 29.81 ms |
| Dispatch() Last Of 10 Routes | 100K | 296.42 ms |
| Dispatch() Last Of 10 Routes | 1M | 2.97 s |
| Dispatch() Last Of 100 Routes | 10K | 215.95 ms |
| Dispatch() Last Of 100 Routes | 100K | 2.16 s |
| Dispatch() Last Of 100 Routes | 1M | 22.27 s |
| Dispatch() Last Of 500 Routes | 10K | 1.05 s |
| Dispatch() Last Of 500 Routes | 100K | 9.62 s |
| Dispatch() Last Of 500 Routes | 1M | 100.91 s |
| Dispatch() Last Of 2000 Routes | 10K | 4.13 s |
| Dispatch() Last Of 2000 Routes | 100K | 41.53 s |
| Dispatch() Last Of 2000 Routes | 1M | 430.89 s |

## Method Convert

| Test | Iteration | FalconHTTP |
|---|---|---|
| MethodFromString() GET (best Case) | 10K | 120.92 us |
| MethodFromString() GET (best Case) | 100K | 1.20 ms |
| MethodFromString() GET (best Case) | 1M | 12.11 ms |
| MethodFromString() OPTIONS (worst Case) | 10K | 267.69 us |
| MethodFromString() OPTIONS (worst Case) | 100K | 2.71 ms |
| MethodFromString() OPTIONS (worst Case) | 1M | 26.95 ms |
| MethodToString() Get | 10K | 54.15 us |
| MethodToString() Get | 100K | 4.88 ms |
| MethodToString() Get | 1M | 6.08 ms |

## Mime Lookup

| Test | Iteration | FalconHTTP |
|---|---|---|
| MimeTypeFromExtension() Known Extension | 10K | 134.38 us |
| MimeTypeFromExtension() Known Extension | 100K | 1.33 ms |
| MimeTypeFromExtension() Known Extension | 1M | 13.61 ms |
| MimeTypeFromExtension() Unknown Extension | 10K | 106.85 us |
| MimeTypeFromExtension() Unknown Extension | 100K | 1.10 ms |
| MimeTypeFromExtension() Unknown Extension | 1M | 10.92 ms |

## Url Decode

| Test | Iteration | FalconHTTP |
|---|---|---|
| UrlDecoder::decode() Mixed Encoding | 10K | 15.47 ms |
| UrlDecoder::decode() Mixed Encoding | 100K | 156.68 ms |
| UrlDecoder::decode() Mixed Encoding | 1M | 1.55 s |
