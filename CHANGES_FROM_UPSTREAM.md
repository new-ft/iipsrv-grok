# Changes From Upstream iipsrv

This fork is based on upstream iipsrv
(https://github.com/ruven/iipsrv) commit `cf302322874158e1f358f8e67d4f94c2acf7e031`
("Added extra check when accessing FCGI REQUEST_METHOD parameter...").

All modifications below are our own and are licensed, same as the rest of
iipsrv, under GPLv3.

## 1. WebP / AVIF tile output

- `src/DeepZoom.cc`: the DeepZoom `.dzi` XML descriptor now advertises
  `Format="webp"` by default whenever WebP support is compiled in
  (`HAVE_WEBP`), instead of always advertising `Format="jpg"`. Overridable
  via the `DEEPZOOM_FORMAT` environment variable (e.g. set to `jpg` to
  restore the previous behaviour, or `avif`/`png`).
- WebP (`HAVE_WEBP`) and AVIF (`HAVE_AVIF`) output support, including the
  relevant compressor classes, quality environment variables
  (`WEBP_QUALITY`, `AVIF_QUALITY`, `AVIF_CODEC`), and Visual Studio project
  wiring (include/lib paths, preprocessor defines) to link libwebp/libavif
  via vcpkg.

## 2. Runtime-selectable JPEG2000 decoder (`JP2_DECODER`)

- `src/FIF.cc`: dispatch logic now supports up to four compile-time-optional
  JPEG2000 decoders (Kakadu, OpenJPEG, Grok, GrokMT) selectable at runtime
  via the `JP2_DECODER` environment variable (`kakadu` / `openjpeg` / `grok`
  / `grokmt`), instead of the decoder being a fixed compile-time choice.
  When only one decoder is compiled in, behaviour is unchanged (that
  decoder is always used).
- `src/Environment.h`: the `JP2_DECODER` *default* value (used when the
  environment variable is unset or invalid) is now derived from which
  decoders are actually compiled into the binary
  (`HAVE_KAKADU` > `HAVE_GROKMT` > `HAVE_GROK` > `HAVE_OPENJPEG`), rather
  than being hardcoded to `"kakadu"`. This preserves identical behaviour for
  builds that include Kakadu, while giving Kakadu-free builds (like
  `ReleasePublic`) a sane, working default.
- `src/OpenJPEGImage.cc`: decode/brightness correctness fixes found while
  validating OpenJPEG against production tiles.
- `src/GrokImage.h` / `src/GrokImage.cc` (**new files**): JPEG2000 decoder
  backed by the Grok codec (https://github.com/GrokImageCompression/grok),
  single-threaded, following the same open/decode/close-per-tile lifecycle
  as the existing Kakadu/OpenJPEG decoders.
- `src/GrokMTImage.h` / `src/GrokMTImage.cc` (**new files**): multi-threaded
  variant of the Grok decoder. Sets `num_threads` (default 4, configurable
  via the `GROK_NUM_THREADS` environment variable) on Grok's single
  `grk_decompress()` call — this is safe and lifecycle-neutral. (Grok's
  "update existing codec" cross-tile codec-reuse API was evaluated
  separately and found to produce incorrect pixel data for this per-request
  tile access pattern; it was **not** adopted — see
  `docs/GROK_EVALUATION.md`.)
- `src/KakaduImage.cc`: added a guard so the file is excluded from
  compilation entirely in configurations that don't define `HAVE_KAKADU`
  (it previously included Kakadu SDK headers unconditionally).

## 3. Memcached made optional / configurable

- A `ReleaseNoMemcached|x64` build configuration (and preprocessor
  conditionals around `HAVE_MEMCACHED` usage) allow building without any
  memcached client code at all, avoiding even a few milliseconds of
  per-request memcached-availability checking overhead when memcached is
  not wanted, rather than relying solely on `MEMCACHED_SERVERS=none` at
  runtime.

## 4. Visual Studio project changes

- `windows/Visual Studio 2022/iipsrv.vcxproj`: added five new build
  configurations beyond upstream's `Debug`/`Release`:
  - `ReleaseNoMemcached|x64` — no memcached code compiled in.
  - `ReleaseGrok|x64` — Kakadu + OpenJPEG + Grok + memcached (comparison
    build).
  - `ReleaseGrokMT|x64` — Kakadu + OpenJPEG + Grok + GrokMT + memcached
    (comparison build).
  - `ReleasePublic|x64` — **the configuration this public repository
    corresponds to**: OpenJPEG + Grok + GrokMT + memcached, WebP/AVIF/PNG
    output. No Kakadu anywhere (no defines, no include/lib paths, no linked
    libraries) — this is the config safe to build and redistribute as the
    binary matching this repository's license obligations.
  - `third_party/grok` and (optional, unused by `ReleasePublic`)
    `third_party/kakadu` are now referenced via the `GrokRoot`/`KakaduRoot`
    MSBuild properties (overridable via environment variable) instead of
    hardcoded, developer-machine-specific absolute paths.

## 5. Not modified

- Grok itself (`third_party/grok` submodule) is used **unmodified** at the
  pinned commit/tag recorded in `.gitmodules`.
- OpenJPEG, libwebp, libpng, libtiff, libjpeg-turbo, zlib, libavif, fcgi,
  libmemcached are all used unmodified, as installed via vcpkg.

Grok pinned version: `v20.4.19` (commit `31ae6fa25be3747370b47a65d311e355f2adb33f`).
