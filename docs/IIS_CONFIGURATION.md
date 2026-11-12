# IIS FastCGI Configuration

This describes how to run the `ReleasePublic` build (OpenJPEG + Grok +
GrokMT + memcached) under IIS via FastCGI, with all runtime behaviour
(decoder choice, thread count, memcached, output quality) driven by
`environmentVariables` entries rather than rebuilding the binary.

All paths, hostnames, and credentials below are illustrative placeholders —
replace them with your own deployment's values; do not reuse them verbatim.

## 1. Enable CGI / FastCGI

Enable the "CGI" Windows feature (IIS also uses this for FastCGI hosting),
then configure a FastCGI application in `applicationHost.config` (or via
IIS Manager > "FastCGI Settings").

```xml
<system.webServer>
  <fastCgi>
    <application fullPath="C:\inetpub\iipsrv\fcgi-bin\iipsrv.exe"
                  arguments=""
                  maxInstances="4"
                  idleTimeout="300"
                  activityTimeout="60"
                  requestTimeout="60"
                  instanceMaxRequests="10000"
                  protocol="NamedPipe"
                  flushNamedPipe="false">
      <environmentVariables>
        <!-- Decoder selection: openjpeg / grok / grokmt
             (kakadu is NOT available in this build) -->
        <environmentVariable name="JP2_DECODER" value="grokmt" />
        <environmentVariable name="GROK_NUM_THREADS" value="4" />

        <!-- Memcached tile cache: "none" to disable -->
        <environmentVariable name="MEMCACHED_SERVERS" value="127.0.0.1:11211" />
        <environmentVariable name="MEMCACHED_TIMEOUT" value="86400" />

        <!-- Output format / quality -->
        <environmentVariable name="DEEPZOOM_FORMAT" value="webp" />
        <environmentVariable name="WEBP_QUALITY" value="50" />
        <environmentVariable name="JPEG_QUALITY" value="75" />
        <environmentVariable name="AVIF_QUALITY" value="50" />

        <!-- Logging -->
        <environmentVariable name="VERBOSITY" value="1" />
        <environmentVariable name="LOGFILE" value="C:\inetpub\iipsrv\logs\iipsrv.log" />

        <!-- Caching -->
        <environmentVariable name="MAX_IMAGE_CACHE_SIZE" value="100" />
        <environmentVariable name="FILESYSTEM_PREFIX" value="C:\images\" />
      </environmentVariables>
    </application>
  </fastCgi>
</system.webServer>
```

## 2. Map the handler

```xml
<system.webServer>
  <handlers>
    <add name="iipsrv-fcgi"
         path="iipsrv.fcgi"
         verb="*"
         modules="FastCgiModule"
         scriptProcessor="C:\inetpub\iipsrv\fcgi-bin\iipsrv.exe"
         resourceType="Unspecified" />
  </handlers>
</system.webServer>
```

## 3. Deploy the binary and its dependencies

Copy into the folder referenced above (`fcgi-bin\`):

- `iipsrv.fcgi`, `iipsrv.exe`
- All vcpkg-managed dependency DLLs produced by the `ReleasePublic` build
  (libwebp/libpng/libtiff/libjpeg-turbo/zlib/liblzma/fcgi/libmemcached/
  libhashkit/libavif/libyuv/openjp2)
- `grokj2k.dll` (from the Grok build, see `docs/BUILD_WINDOWS.md`)

Recycle the application pool (or restart the FastCGI process) after any
environment variable change — iipsrv reads these only at process startup.

## 4. Full list of supported environment variables

| Variable | Purpose | Example / default |
|---|---|---|
| `JP2_DECODER` | JPEG2000 decoder: `openjpeg` / `grok` / `grokmt` | `grokmt` |
| `GROK_NUM_THREADS` | Threads per `grokmt` decode call | `4` |
| `MEMCACHED_SERVERS` | Tile cache servers, or `none` | `127.0.0.1:11211` |
| `MEMCACHED_TIMEOUT` | Cache entry TTL (seconds) | `86400` |
| `DEEPZOOM_FORMAT` | DeepZoom `.dzi` advertised tile format | `webp` (default when `HAVE_WEBP`), `jpg`, `png`, `avif` |
| `WEBP_QUALITY` | WebP encode quality (0-100) | `50` |
| `SOURCE_CODE_URL` | Source-offer URL sent in `Link`/`X-Source-Code` headers (empty disables) | repo release tag URL |
| `JPEG_QUALITY` | JPEG encode quality (0-100) | `75` |
| `PNG_QUALITY` | PNG compression level (0-9) | `6` |
| `AVIF_QUALITY` | AVIF encode quality (0-100) | `50` |
| `AVIF_CODEC` | AVIF codec backend | implementation default |
| `TIFF_COMPRESSION` / `TIFF_QUALITY` | Source TIFF handling | implementation default |
| `MAX_IMAGE_CACHE_SIZE` | In-process decoded-image cache size (MB) | `10` |
| `MAX_METADATA_CACHE_SIZE` | Metadata cache size (MB) | `1000` |
| `FILENAME_PATTERN` | Multi-page/sequence filename pattern | implementation default |
| `FILESYSTEM_PREFIX` / `FILESYSTEM_SUFFIX` | Path prefix/suffix applied to requested filenames | empty |
| `MAX_LAYERS` / `MAX_CVT` | Resolution/conversion limits | implementation default |
| `WATERMARK` / `WATERMARK_PROBABILITY` / `WATERMARK_OPACITY` | Watermark overlay | disabled by default |
| `INTERPOLATION` | Resampling filter | implementation default |
| `CORS` | CORS header policy | disabled by default |
| `BASE_URL` | Base URL used in generated responses | empty |
| `CACHE_CONTROL` | `Cache-Control` header value | implementation default |
| `ALLOW_UPSCALING` | Allow requesting sizes larger than native | `0` |
| `URI_MAP` | Custom request-path mapping | none |
| `MAX_ICC` / `MAX_EXIF` | Embedded metadata size limits | implementation default |
| `CODEC_PASSTHROUGH` | Pass through source codec without re-encode where possible | implementation default |
| `VERBOSITY` | Log verbosity (0-6) | `1` |
| `LOGFILE` | Log file path | stderr |
| `IIIF_VERSION` / `IIIF_DELIMITER` / `IIIF_EXTRA_INFO` / `IIIF_EXTENSIONS` | IIIF protocol options | implementation default |
| `COPYRIGHT` | Copyright string embedded in responses | empty |

`KAKADU_READMODE` and `JP2_DECODER=kakadu`/`DECODER_MODULES` (dynamically
loaded decoder modules) are recognized by upstream iipsrv but are
**not applicable** to this build — `ReleasePublic` has no Kakadu support
compiled in; setting `JP2_DECODER=kakadu` will cause the server to log
`Unknown or unavailable JP2_DECODER setting: kakadu` and fail to serve
JPEG2000 images (verified behaviour, not a crash).
