# iipsrv — OpenJPEG + Grok (Public Build)

This is a public-repository build of [IIPImage Server](https://github.com/ruven/iipsrv)
(`iipsrv`), extended with:

- **WebP and AVIF tile output** (in addition to JPEG/PNG), including a
  DeepZoom `.dzi` descriptor patch that advertises WebP by default.
- **Runtime-selectable JPEG2000 decoding** between **OpenJPEG**, **Grok**,
  and **GrokMT** (multi-threaded Grok) — no rebuild required, just set the
  `JP2_DECODER` environment variable.
- **Optional memcached-backed tile caching**, fully config-controlled
  (`MEMCACHED_SERVERS`).
- A bug fix for an OpenJPEG decode-region regression present in upstream
  iipsrv (see `CHANGES_FROM_UPSTREAM.md`).

**Kakadu is intentionally excluded** from this build and repository: Kakadu
is a separately, commercially licensed JPEG2000 SDK that cannot be
redistributed. The `ReleasePublic` configuration documented here does not
reference or require it at all — see `SOURCE_OFFER.md` for the full
rationale.

The original upstream project README is preserved at
[`README.upstream.md`](README.upstream.md).

## Why this repository exists

This combination (GPLv3 iipsrv + AGPLv3 Grok) triggers the AGPLv3's
network-use source-disclosure clause. This repository **is** that
disclosure: the complete corresponding source, build scripts, and
configuration documentation needed to reproduce the exact binary this
project runs in production. See:

- [`NOTICE`](NOTICE) — licensing summary
- [`LICENSE`](LICENSE) — GPLv3 (iipsrv)
- [`LICENSE-AGPL`](LICENSE-AGPL) — AGPLv3 (Grok)
- [`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md) — all other dependencies
- [`SOURCE_OFFER.md`](SOURCE_OFFER.md) — what is/isn't included, and why
- [`CHANGES_FROM_UPSTREAM.md`](CHANGES_FROM_UPSTREAM.md) — itemized diff vs. upstream iipsrv

## Quick start

```powershell
git clone --branch public/openjpeg-grok-multithreaded <this-repo-url> iipsrv-public
cd iipsrv-public
git submodule update --init --recursive
```

**New to this repo? Start with [`docs/DEVELOPER_ONBOARDING.md`](docs/DEVELOPER_ONBOARDING.md)**
— a single, self-contained checklist covering prerequisites, building
(whether you received this repo via `git clone` or as a plain shared
folder), running a local smoke test, and a troubleshooting quick
reference.

For deeper detail, see [`docs/BUILD_WINDOWS.md`](docs/BUILD_WINDOWS.md)
(full build reference) and [`docs/IIS_CONFIGURATION.md`](docs/IIS_CONFIGURATION.md)
(IIS deployment).

## Configuration

All configuration is via environment variables (process-level, or
FastCGI `environmentVariables` entries under IIS) — no source changes or
rebuild needed to change behavior. The most relevant to this build:

| Variable | Purpose | Values |
|---|---|---|
| `JP2_DECODER` | Active JPEG2000 decoder | `openjpeg` / `grok` / `grokmt` |
| `GROK_NUM_THREADS` | Thread count for `grokmt` | integer, default `4` |
| `DEEPZOOM_FORMAT` | Tile format advertised in DeepZoom `.dzi` | e.g. `webp` (build default), `jpg`, `png` |
| `WEBP_QUALITY` | WebP tile quality | integer, default `50` |
| `SOURCE_CODE_URL` | URL sent in `Link: rel="source"` and `X-Source-Code` headers (AGPLv3 §13) | default: this repo's release tag; empty disables |
| `MEMCACHED_SERVERS` | Tile cache servers, or disable | e.g. `127.0.0.1:11211`, or `none` |

See [`docs/IIS_CONFIGURATION.md`](docs/IIS_CONFIGURATION.md) for the
complete list of all supported environment variables (quality settings,
caching limits, watermarking, IIIF options, etc.).

## Decoder evaluation / rationale

See [`docs/GROK_EVALUATION.md`](docs/GROK_EVALUATION.md) for the full
Kakadu vs. OpenJPEG vs. Grok vs. GrokMT benchmark methodology and results
that led to this build's decoder set and defaults.

## Repository layout

```
src/                       iipsrv source (incl. GrokImage.*, GrokMTImage.*)
fcgi/                      bundled libfcgi source
windows/                   Visual Studio solution/project (ReleasePublic|x64 config)
third_party/grok/          Grok JPEG2000 codec (git submodule, pinned commit)
docs/
  DEVELOPER_ONBOARDING.md   single checklist: prerequisites, build, smoke-test, troubleshooting
  BUILD_WINDOWS.md           full step-by-step build instructions
  IIS_CONFIGURATION.md       IIS FastCGI deployment + full env var reference
  GROK_EVALUATION.md         decoder benchmark methodology/results
NOTICE, LICENSE, LICENSE-AGPL,
THIRD_PARTY_LICENSES.md, SOURCE_OFFER.md, CHANGES_FROM_UPSTREAM.md
  licensing / compliance documentation
```

## Upstream

This is a fork of [ruven/iipsrv](https://github.com/ruven/iipsrv), based on
commit `cf302322874158e1f358f8e67d4f94c2acf7e031`. All modifications are
documented in [`CHANGES_FROM_UPSTREAM.md`](CHANGES_FROM_UPSTREAM.md).

## Maintainer and licence enquiries

Maintainer: [@cbgrasshopper](https://github.com/cbgrasshopper). Licence and source-code enquiries: open an issue at https://github.com/new-ft/iipsrv-grok/issues.
