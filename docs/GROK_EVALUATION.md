# Grok Decoder Evaluation (OpenJPEG vs. Grok vs. GrokMT vs. Kakadu)

This document summarizes the evaluation that led to adopting Grok and
GrokMT as alternative JPEG2000 decoders in this project, run on an internal
test image (`<path>\100001527.jp2`, 3059 x 4517, single channel, 8-bit) and
54 distinct tile requests at full resolution per decoder (identical
methodology across all runs, to keep the comparison fair). Absolute file
paths and machine-specific details have been generalized; the comparative
results and conclusions are unchanged from the original evaluation.

## 1. Single-threaded comparison: Kakadu vs. OpenJPEG vs. Grok

All three decoders were compiled into one binary, selected at runtime via
`JP2_DECODER`, tested on the same machine/binary/moment.

| Decoder  | Mean (µs) | Median (µs) | Min (µs) | Max (µs) | StdDev (µs) |
|----------|----------:|------------:|---------:|---------:|------------:|
| Kakadu   |     6,248 |       6,028 |    2,951 |   11,942 |       2,016 |
| OpenJPEG |    15,710 |      15,010 |    7,024 |   29,027 |       5,421 |
| Grok     |    17,804 |      17,012 |    9,999 |   26,993 |       4,640 |

- Output was verified byte-for-byte identical (SHA-256) between decoders at
  several tiles/resolutions — pixel correctness, not just HTTP 200, was
  required before trusting any timing.
- Single-threaded Grok (`grk_initialize(NULL, 1, NULL)`) was **slower** than
  both Kakadu (~2.8x) and OpenJPEG (~13%) in this workload, contradicting
  Grok's own "fastest JPEG2000 codec" marketing claim for this use case.

**Conclusion at this stage: do not adopt Grok (single-threaded) for
production; Kakadu remained the performance leader, OpenJPEG the best
open-source option.**

## 2. Follow-up: multi-threaded Grok (GrokMT)

The single-threaded evaluation noted an open caveat: Grok supports
additional multi-threaded decode paths and an "update existing codec" API
that were intentionally not used, to keep the first comparison
architecturally fair to the other (single-threaded, open/decode/close
per-tile) decoders. This was investigated as a follow-up.

### 2a. Codec reuse ("update existing codec" API) — rejected, proven unsafe

`grk_decompress_update()` is documented valid only "before the first
`grk_decompress()` call." A standalone test opened one codec, decoded
window A, called `grk_decompress_update()` to switch to window B, then
decoded again on the same codec:

- Window A matched the known-good baseline.
- **Window B did NOT match** the baseline — the call returned success but
  produced incorrect/stale pixel data.

**Codec reuse across distinct tile windows is not safely usable** via
Grok's current public API for iipsrv's per-request access pattern (every
request opens a fresh codec). This was not pursued; no changes were made to
the per-request open/decode/close lifecycle.

### 2b. Multi-threaded single-call decode — adopted, proven safe and effective

Setting `num_threads > 1` for a single `grk_decompress()` call requires
**no lifecycle change**. A standalone test decoded the same window
repeatedly with `num_threads` in {1, 2, 4, 8}:

- All thread counts produced **identical, correct** output.
- At a production-realistic 256x256 tile size: 1 thread ~9.6ms,
  2 ~7.5ms, **4 ~7.1ms (optimal, ~26% faster than 1 thread)**,
  8 ~8.3ms (regression - thread-pool oversubscription).

This became the `GrokMTImage` decoder (`src/GrokMTImage.h`/`.cc`), differing
from `GrokImage` only in the thread count passed to
`grk_decompress_init()` (default 4, configurable via `GROK_NUM_THREADS`).

### End-to-end (server-level) results, 54 tiles per decoder

| Decoder                | Mean (us) | Median (us) | Min (us) | Max (us) | StdDev (us) |
|--------------------------|----------:|------------:|---------:|---------:|------------:|
| Kakadu                   |     8,363 |       4,992 |    2,997 |  112,005 |      14,612 |
| OpenJPEG                 |    18,096 |      16,000 |    8,003 |   50,000 |       8,500 |
| Grok (1 thread)          |    16,988 |      15,757 |    8,027 |   36,990 |       5,974 |
| **GrokMT (4 threads)**   | **12,912** | **10,506** |    5,971 |   44,001 |       5,957 |

- GrokMT vs. single-threaded Grok: **~24% faster (mean)**, **~33% faster
  (median)**, matching the isolated raw-decode-only test.
- GrokMT **overtakes OpenJPEG** (median 10,506us vs. 16,000us) - reversing
  the earlier conclusion that Grok was strictly worse than both
  alternatives.
- GrokMT remains **slower than Kakadu** by roughly 2x (median).
- GrokMT output was verified byte-for-byte identical (SHA-256) to the
  known-good single-threaded Grok baseline for the same tile.

## Overall conclusion

- **Kakadu remains the fastest decoder** if its commercial license is
  acceptable.
- If a free/open-source-only decoder is required (as in the
  `ReleasePublic` build, which excludes Kakadu entirely),
  **GrokMT (4 threads) is the recommended choice**, ahead of both
  single-threaded Grok and OpenJPEG in this workload.
- Codec reuse across tile windows was empirically proven **unsafe** with
  Grok's current public API and was not adopted.

## Methodology notes

- Decode time was taken from the server's own log line
  (`TileManager :: Tile decoding time: N microseconds`), not client
  wall-clock, since request/socket overhead dwarfs actual decode time.
- Absolute numbers vary with machine load; the GrokMT-vs-Grok relative
  comparison (same machine, same moment, repeated runs) is the reliable,
  reproducible finding.
