# Source Offer (AGPLv3 §13 Compliance)

This repository **is** the complete, corresponding source code for the
`ReleasePublic` build of iipsrv as deployed, satisfying the AGPLv3 "network
use" source-disclosure requirement (AGPLv3 Section 13) that is triggered by
combining GPLv3-licensed iipsrv with AGPLv3-licensed Grok.

## What is included here

1. **Full modified iipsrv source** (`src/`, `fcgi/`, `windows/`) — this is a
   fork of the upstream iipsrv project (https://github.com/ruven/iipsrv),
   with our changes isolated on the `public/openjpeg-grok-multithreaded`
   branch. See `CHANGES_FROM_UPSTREAM.md` for an itemized list of every
   modification and the upstream commit this fork is based on.
2. **The exact version of Grok used**, referenced as the `third_party/grok`
   git submodule pinned to a specific commit/tag — not a copy, since Grok
   itself is unmodified; this makes the exact upstream source trivially
   reproducible (`git submodule update --init`).
3. **Our Grok integration code** (`src/GrokImage.h`, `src/GrokImage.cc`,
   `src/GrokMTImage.h`, `src/GrokMTImage.cc`) — these are our own new files
   (not modifications to Grok itself) that call Grok's public C API; they
   are part of iipsrv's own GPLv3-licensed source and fully included here.
4. **Complete build scripts**: the Visual Studio solution/project
   (`windows/Visual Studio 2022/iipsrv.vcxproj`, `iipsrv-MSVC2022.sln`) with
   the `ReleasePublic|x64` configuration that produces the exact binary
   described above, plus `docs/BUILD_WINDOWS.md` with step-by-step
   instructions (vcpkg packages, Grok submodule build, MSBuild invocation).
5. **Build-time configuration** — every environment variable consulted by
   the binary at runtime is documented in `README.md` (decoder selection,
   quality settings, memcached, thresholds, etc.) so the exact running
   configuration can be reproduced.

## What is intentionally NOT included

- **Kakadu SDK source/binaries.** Kakadu is a separately, commercially
  licensed JPEG2000 SDK. It is not AGPL/GPL licensed, we have no right to
  redistribute it, and the `ReleasePublic` configuration published here does
  not use it at all (`HAVE_KAKADU` is not defined; see
  `CHANGES_FROM_UPSTREAM.md`). The upstream iipsrv `KakaduImage.cc`/`.h`
  integration files remain present in `src/` (as they are in the original
  upstream project) purely for transparency/completeness of the GPLv3 iipsrv
  codebase, but are excluded from the `ReleasePublic` build
  (`ExcludedFromBuild` in the vcxproj) and require a separately obtained
  Kakadu SDK to compile at all. No proprietary Kakadu code or binaries are
  distributed in this repository.
- **Client-specific / deployment-specific content**: no production image
  files, no internal server hostnames/IP addresses, no credentials, no
  proprietary CMS/viewer front-end code, and no memcached server addresses
  beyond illustrative examples (e.g. `127.0.0.1:11211`).

## How to obtain this source

This repository itself is the offer: anyone with access to it (or to a
public hosting of it, e.g. GitHub) already has the complete corresponding
source. If this repository is provided to you as a plain file bundle (not
via a public git host), you may request the latest version of this exact
source offer, free of charge beyond reasonable distribution cost, from the
operator of the service deploying this software, as required by AGPLv3
Section 13.

## Release record

| Item | Value |
|---|---|
| Release tag | `iipsrv-1.3-grok-20.4.19` |
| Source repository | https://github.com/new-ft/iipsrv-grok |
| Branch | `public/openjpeg-grok-multithreaded` |
| Upstream iipsrv commit | `cf302322874158e1f358f8e67d4f94c2acf7e031` (https://github.com/ruven/iipsrv) |
| Grok version | `v20.4.19` (commit `31ae6fa25be3747370b47a65d311e355f2adb33f`), unmodified, submodule `third_party/grok` |

Maintainer contact: [@cbgrasshopper](https://github.com/cbgrasshopper)
