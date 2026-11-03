# Third-Party Licenses

This build links against the following third-party libraries. None of these
libraries have been modified by this project; they are used as unmodified
upstream dependencies (installed via vcpkg, except Grok which is a pinned
git submodule). Their full license texts are included in each package itself
(vcpkg package `share/<port>/copyright`, or `third_party/grok/LICENSE`).

| Library | Purpose | License | Upstream |
|---|---|---|---|
| Grok (`grokj2k`) | JPEG2000 decode (Grok / GrokMT decoders) | GNU AGPL v3 | https://github.com/GrokImageCompression/grok |
| OpenJPEG (`openjp2`) | JPEG2000 decode (OpenJPEG decoder) | 2-Clause BSD | https://github.com/uclouvain/openjpeg |
| libwebp, libsharpyuv, libwebpmux | WebP tile encoding | BSD-style (Google) | https://chromium.googlesource.com/webm/libwebp |
| libavif, libyuv | AVIF tile encoding | BSD-2-Clause | https://github.com/AOMediaCodec/libavif |
| libpng | PNG tile encoding | libpng license | http://www.libpng.org/pub/png/libpng.html |
| libtiff | TIFF image reading | libtiff license (BSD-like) | http://www.libtiff.org/ |
| libjpeg-turbo | JPEG tile encoding | IJG License / BSD-style | https://libjpeg-turbo.org/ |
| zlib | Compression (PNG/TIFF dependency) | zlib License | https://zlib.net/ |
| liblzma (xz) | Compression (TIFF dependency) | Public Domain | https://tukaani.org/xz/ |
| fcgi | FastCGI protocol library | OSI-approved FastCGI license | https://github.com/FastCGI-Archives/fcgi2 |
| libmemcached, libhashkit | Memcached tile-cache client | BSD | https://libmemcached.org/ |

## How dependencies are obtained

- All libraries above **except Grok** are installed via vcpkg in classic
  (non-manifest) mode:

  ```powershell
  vcpkg install fcgi:x64-windows libpng:x64-windows libwebp:x64-windows `
    libtiff:x64-windows libjpeg-turbo:x64-windows liblzma:x64-windows `
    zlib:x64-windows libmemcached:x64-windows libavif:x64-windows `
    openjpeg:x64-windows
  vcpkg integrate install
  ```

- **Grok** is included as the `third_party/grok` git submodule, pinned to a
  specific upstream commit/tag (see `.gitmodules` and `docs/BUILD_WINDOWS.md`
  for the exact version and build steps). It is built separately with CMake
  and the resulting `grokj2k.lib`/`grokj2k.dll` are linked into `iipsrv.exe`.

Per the AGPLv3 and GPLv3 terms, the license text of each dependency remains
in full force; this table is a convenience summary only and does not replace
the authoritative texts shipped with each package.

Grok pinned version: `v20.4.19` (commit `31ae6fa25be3747370b47a65d311e355f2adb33f`), AGPLv3.
