# Building on Windows (ReleasePublic configuration)

> New developer? [`docs/DEVELOPER_ONBOARDING.md`](DEVELOPER_ONBOARDING.md)
> is a shorter, single-pass checklist covering the same ground plus a
> smoke test. This document is the fuller reference.

These instructions build the `ReleasePublic|x64` configuration: OpenJPEG +
Grok + GrokMT decoders, memcached tile-cache support, WebP/AVIF/PNG/JPEG
output — **no Kakadu** (Kakadu is a separately licensed SDK and is not
required by, or usable with, this configuration).

## Prerequisites

- Windows 10/11 x64
- Visual Studio 2022 (Desktop development with C++ workload)
- [CMake](https://cmake.org/) >= 3.24 (to build the Grok submodule)
- [vcpkg](https://github.com/microsoft/vcpkg) (classic, non-manifest mode)
- Git

## 1. Clone this repository and its submodule

```powershell
git clone --branch public/openjpeg-grok-multithreaded <this-repo-url> iipsrv-public
cd iipsrv-public
git submodule update --init --recursive
```

This fetches Grok into `third_party\grok`, pinned to the exact commit
recorded in `.gitmodules` (the same version this fork was built and tested
against).

> If you were instead handed this directory as a plain file copy (not a
> git clone), initialize it as a repository first (`git init`,
> `git add -A`, `git commit`) before running the submodule commands, or
> simply `git clone https://github.com/GrokImageCompression/grok.git
> third_party\grok` and `git checkout <commit>` inside it to match the
> pinned version recorded in `.gitmodules`.

> **Known gotcha — `git clone --recursive` hangs on Grok.** Grok's
> `.gitmodules` includes `extern/grok-gpu-plugin`, which points at an
> internal SSH URL (`git@gitlab-rsa:...`) requiring credentials you don't
> have; a naive recursive clone stalls indefinitely waiting on that
> auth/network. The submodule command above (`--recursive`, as used by
> `git submodule update --init --recursive` against *this* repo's own
> `.gitmodules`) is fine since we don't recurse into Grok's own
> submodules that way. If you ever clone Grok's upstream repository
> directly (rather than via this repo's pinned submodule) and need its
> own public submodules (`CLI11`, `spdlog`, `taskflow`, `highway`), fetch
> it as `git clone --depth 1` (non-recursive) followed by
> `git submodule update --init --depth 1 -- src/include/CLI11 src/lib/core/highway src/include/spdlog src/include/taskflow`,
> explicitly skipping `extern/grok-gpu-plugin` (an optional, proprietary
> GPU plugin not needed for iipsrv's CPU-only integration).

## 2. Install open-source dependencies via vcpkg

```powershell
cd C:\path\to\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install fcgi:x64-windows libpng:x64-windows libwebp:x64-windows `
  libtiff:x64-windows libjpeg-turbo:x64-windows liblzma:x64-windows `
  zlib:x64-windows libmemcached:x64-windows libavif:x64-windows `
  openjpeg:x64-windows
.\vcpkg integrate install
```

> **Known "newer package version" gotcha:** early in this project, tiles at
> deeper DeepZoom zoom levels returned `500`/`404` errors
> (`OpenJPEG error :: Size x of the decoded component image is incorrect`)
> after rebuilding against vcpkg's OpenJPEG. This looked like an OpenJPEG
> *version* regression (the original prebuilt binary used 2.5.3, vcpkg
> installed 2.5.4), so OpenJPEG 2.5.3 was built from source and swapped in
> as a test — **the bug persisted identically**, ruling out the library
> version. The real cause was a pre-existing bug in iipsrv's own
> `OpenJPEGImage.cc`: its decode-region clamp logic re-derived an absolute
> end-coordinate as a relative width whenever a tile's edge exceeded the
> image bounds, producing an invalid region passed to
> `opj_set_decode_area()`. This is already fixed in this fork (see
> `CHANGES_FROM_UPSTREAM.md`); it's noted here because it's a good example
> of why a "it broke after a dependency update" symptom is worth verifying
> against the actual library version before assuming the dependency is at
> fault.

## 3. Build Grok

```powershell
cd third_party\grok
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target grokj2k
```

This produces `third_party\grok\build\bin\grokj2k.dll` and the import
library used at link time. (If your CMake/Grok version places the library
elsewhere, note the path — you will pass it via `GrokRoot` in step 4 if it
differs from the default.)

> **Known gotcha — only build the `grokj2k` target.** Grok's CMake also
> defines a `grokj2kcodec` target (CLI tools: `grk_compress`,
> `grk_decompress`, `grk_transcode`) that can fail to link with
> `LNK2005`/`LNK1169` duplicate zlib symbols, because it pulls in both
> vcpkg's `z.lib` and Grok's own internally-built static `zlibstatic.lib`
> transitively (Grok's CMake mostly builds its own bundled copies of
> zlib/libjpeg-turbo/libpng/libtiff rather than reusing vcpkg's). This is
> harmless to ignore: iipsrv only calls Grok's core C API
> (`grok.h`/`grokj2k`), not its CLI tools, so building just the `grokj2k`
> target (as shown above) avoids the issue entirely.

## 4. Build iipsrv (`ReleasePublic|x64`)

Open `windows\Visual Studio 2022\iipsrv-MSVC2022.sln` in Visual Studio 2022,
select configuration **ReleasePublic**, platform **x64**, and build
(Ctrl+Shift+B). Or from the command line:

```powershell
msbuild "windows\Visual Studio 2022\iipsrv.vcxproj" /p:Configuration=ReleasePublic /p:Platform=x64
```

By default, the project expects Grok at
`<repo-root>\third_party\grok` (source) and
`<repo-root>\third_party\grok\build` (CMake build output) — i.e. exactly
where steps 1 and 3 place it. To use a different location, set the
`GrokRoot` environment variable before building:

```powershell
$env:GrokRoot = "D:\deps\grok"
msbuild ... /p:Configuration=ReleasePublic /p:Platform=x64
```

The resulting binary is at
`windows\Visual Studio 2022\ReleasePublic\x64\iipsrv.fcgi` (and
`iipsrv.exe`), alongside the vcpkg-deployed dependency DLLs
(`vcpkg.applocal.log` lists what was copied) and `grokj2k.dll` (copy this
one manually from the Grok build output if it is not auto-deployed).

## 5. Deploy

Copy the following to your deployment folder (e.g. `fcgi-bin\`):

- `iipsrv.fcgi`, `iipsrv.exe`
- All vcpkg-deployed DLLs from the build output folder
- `grokj2k.dll` from `third_party\grok\build\bin`

See `README.md` for runtime configuration (environment variables) and
`docs/IIS_CONFIGURATION.md` for IIS FastCGI setup.

## Other configurations in this project (not required for the public build)

The same `.vcxproj` also defines `ReleaseGrok|x64` and `ReleaseGrokMT|x64`,
which additionally compile in Kakadu support for side-by-side decoder
comparison (see `docs/GROK_EVALUATION.md`). These configurations require a
separately obtained, commercially licensed Kakadu SDK
(set `KakaduRoot` to its location) and are **not** part of this public
source offer's distributed binary; they are included only for transparency
of the full development history. You do not need Kakadu to build or run
`ReleasePublic`.
