# Developer Onboarding

This is the single checklist for a new developer to get this repository
building locally and verify it works, whether you received it via `git
clone` or as a plain shared folder (zip / network share / etc.).

## 0. How was this repo given to you?

- **Git clone / GitHub access** → go to [Path A](#path-a-you-have-git-access-to-this-repository).
- **Plain folder copy** (zip, shared drive, USB, etc., no `.git` history
  usable, or submodule not fetched) → go to [Path B](#path-b-you-received-a-plain-folder-copy).

Either way, install the prerequisites in [Step 1](#1-install-prerequisites) first.

## 1. Install prerequisites

| Tool | Version | Why |
|---|---|---|
| Windows 10/11 x64 | — | target platform |
| [Visual Studio 2022](https://visualstudio.microsoft.com/) | Desktop development with C++ workload | builds `iipsrv.exe`/`iipsrv.fcgi` |
| [CMake](https://cmake.org/download/) | >= 3.24 | builds the Grok submodule |
| [Git](https://git-scm.com/) | any recent | fetches/pins the Grok submodule |
| [vcpkg](https://github.com/microsoft/vcpkg) | classic (non-manifest) mode | installs all other open-source dependencies |

Install vcpkg once:

```powershell
git clone https://github.com/microsoft/vcpkg C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install fcgi:x64-windows libpng:x64-windows libwebp:x64-windows `
  libtiff:x64-windows libjpeg-turbo:x64-windows liblzma:x64-windows `
  zlib:x64-windows libmemcached:x64-windows libavif:x64-windows `
  openjpeg:x64-windows
.\vcpkg integrate install
```

> If `libmemcached`/`libevent`-style dependency build errors ever appear
> after a vcpkg update, see the version-gotcha note in
> `docs/BUILD_WINDOWS.md` before assuming your toolchain is broken.

## Path A: you have git access to this repository

```powershell
git clone --branch public/openjpeg-grok-multithreaded <repo-url> iipsrv-public
cd iipsrv-public
git submodule update --init --recursive
```

This fetches Grok into `third_party\grok`, already pinned to the exact
commit this build was developed and tested against — nothing further to
configure. Skip to [Step 2](#2-build-grok).

## Path B: you received a plain folder copy

`third_party\grok` in a plain folder copy is likely **empty** (it's a git
submodule reference, not vendored source). Fetch Grok yourself, pinned to
the same commit recorded in `.gitmodules`:

```powershell
git clone https://github.com/GrokImageCompression/grok.git third_party\grok
cd third_party\grok
git checkout 31ae6fa25be3747370b47a65d311e355f2adb33f
cd ..\..
```

> **Known gotcha:** do **not** `git clone --recursive` Grok's own repo —
> it hangs indefinitely on a private SSH submodule
> (`extern/grok-gpu-plugin`) that isn't needed here. See
> `docs/BUILD_WINDOWS.md` for the safe non-recursive clone + selective
> submodule-init workaround if you need Grok's own public submodules.

## 2. Build Grok

```powershell
cd third_party\grok
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target grokj2k
cd ..\..
```

Only build the `grokj2k` target (not the full default target) — a
separate `grokj2kcodec` target can fail to link with duplicate zlib
symbols; that target isn't used by iipsrv and the failure is safe to
ignore. Confirm the output exists:

```powershell
Test-Path third_party\grok\build\bin\grokj2k.dll   # should be True
```

## 3. Build iipsrv

Open `windows\Visual Studio 2022\iipsrv-MSVC2022.sln`, select configuration
**ReleasePublic**, platform **x64**, build (Ctrl+Shift+B) — or from the
command line:

```powershell
msbuild "windows\Visual Studio 2022\iipsrv.vcxproj" /p:Configuration=ReleasePublic /p:Platform=x64
```

The project expects Grok at `<repo-root>\third_party\grok` by default. If
yours lives elsewhere, set `GrokRoot` before building:

```powershell
$env:GrokRoot = "D:\deps\grok"
msbuild ... /p:Configuration=ReleasePublic /p:Platform=x64
```

Output: `windows\Visual Studio 2022\ReleasePublic\x64\iipsrv.fcgi`.

## 4. Deploy the dependency DLLs

vcpkg auto-copies its own DLLs next to the binary on build
(`vcpkg.applocal.log` lists what was copied). You must manually copy one
more file that vcpkg doesn't manage:

```powershell
Copy-Item third_party\grok\build\bin\grokj2k.dll "windows\Visual Studio 2022\ReleasePublic\x64\"
```

## 5. Verify the build works

The build output is `iipsrv.fcgi` (a FastCGI binary — plain `curl`/browser
HTTP GET against it in standalone mode will return nothing useful, since it
speaks the binary FastCGI protocol, not HTTP). Copy it to `iipsrv.exe` and
run it in standalone `--bind` mode for a quick local smoke test:

```powershell
cd "windows\Visual Studio 2022\ReleasePublic\x64"
Copy-Item iipsrv.fcgi iipsrv.exe
$env:JP2_DECODER = "grokmt"      # or "openjpeg" / "grok"
$env:MEMCACHED_SERVERS = "none"  # disable memcached for this smoke test
$env:VERBOSITY = "2"
$env:LOGFILE = "iipsrv_test.log"
Start-Process .\iipsrv.exe -ArgumentList "--bind :9999"
```

Then, from the repo root, request a tile from any `.jp2`/`.tif` image you
have locally (adjust `FIF=` to your own test image path). Save this as
`fcgi_smoke_test.py` — it's a minimal, self-contained raw FastCGI client
(standalone `--bind` mode speaks binary FastCGI, not HTTP, so plain
`curl`/browser requests won't work):

```python
import socket, struct, sys

HOST, PORT = "localhost", 9999
QUERY = "FIF=D:/path/to/your/test-image.jp2&JTL=4,0"  # <-- edit this

def record(rtype, content, request_id=1):
    padding = (-len(content)) % 8
    header = struct.pack("!BBHHBx", 1, rtype, request_id, len(content), padding)
    return header + content + b"\x00" * padding

def name_value(name, value):
    return struct.pack("!B", len(name)) + struct.pack("!B", len(value)) + name + value

s = socket.create_connection((HOST, PORT), timeout=10)
s.sendall(record(1, struct.pack("!HB5x", 1, 0)))  # BEGIN_REQUEST, role=RESPONDER

params = b""
for k, v in [("QUERY_STRING", QUERY), ("REQUEST_METHOD", "GET"),
             ("SCRIPT_NAME", "/iipsrv.fcgi"), ("SCRIPT_FILENAME", "/iipsrv.fcgi"),
             ("SERVER_NAME", HOST), ("SERVER_PORT", str(PORT)),
             ("SERVER_PROTOCOL", "HTTP/1.1"), ("GATEWAY_INTERFACE", "CGI/1.1")]:
    params += name_value(k.encode(), v.encode())
s.sendall(record(4, params))   # FCGI_PARAMS
s.sendall(record(4, b""))     # FCGI_PARAMS end
s.sendall(record(5, b""))     # FCGI_STDIN end

data = b""
while True:
    chunk = s.recv(65536)
    if not chunk: break
    data += chunk
s.close()

out, i = b"", 0
while i + 8 <= len(data):
    ver, rtype, rid, length, padding = struct.unpack("!BBHHBx", data[i:i+8])
    i += 8
    content = data[i:i+length]
    i += length + padding
    if rtype == 6:  # STDOUT
        out += content

sep = out.find(b"\r\n\r\n")
print(out[:sep].decode(errors="replace"))
print("body bytes:", len(out) - sep - 4)
```

```powershell
python fcgi_smoke_test.py
```

Check `iipsrv_test.log` for a line like:



```
FIF :: Using grokmt JPEG2000 decoder
TileManager :: Tile decoding time: N microseconds
```

That confirms the decoder loaded and successfully decoded a tile. Stop the
test process afterward (`Stop-Process` by PID, or Ctrl+C if run in the
foreground without `Start-Process`).

## 6. Where to go next

| If you need to... | See |
|---|---|
| Deploy under IIS (FastCGI config, all environment variables) | `docs/IIS_CONFIGURATION.md` |
| Understand why Grok/GrokMT/OpenJPEG were chosen over Kakadu, with benchmarks | `docs/GROK_EVALUATION.md` |
| See exactly what changed vs. upstream iipsrv | `CHANGES_FROM_UPSTREAM.md` |
| Understand the licensing situation (why this repo exists at all) | `README.md`, `NOTICE`, `SOURCE_OFFER.md` |
| Full build reference / troubleshooting | `docs/BUILD_WINDOWS.md` |

## Troubleshooting quick reference

| Symptom | Cause | Fix |
|---|---|---|
| `fatal error: jpx.h: No such file` or similar Kakadu header errors while building `ReleasePublic` | You're building the wrong configuration, or a stray `HAVE_KAKADU` define leaked in | Make sure **Configuration = ReleasePublic**, not `ReleaseGrok`/`ReleaseGrokMT`/`Release`/`Debug` (those require the separately-licensed Kakadu SDK and are not part of this public build) |
| `LNK2001`/`LNK2019` referencing `grk_*` symbols | Grok wasn't built, or `GrokRoot` points to the wrong place | Re-run step 2; verify `third_party\grok\build\bin\grokj2k.dll` exists; set `$env:GrokRoot` explicitly if Grok lives elsewhere |
| `git clone --recursive` on Grok hangs forever | Grok's private `extern/grok-gpu-plugin` SSH submodule | Don't use `--recursive` on Grok directly; follow Path B's `git checkout <pinned-commit>` instead, which doesn't need Grok's own submodules at all for this build |
| Server starts but `JP2_DECODER=kakadu` throws `Unknown or unavailable JP2_DECODER setting: kakadu` | Expected — `ReleasePublic` has no Kakadu support | Use `openjpeg`, `grok`, or `grokmt` instead |
| `vcpkg install` fails on a dependency after a vcpkg update | A newer port version can regress on Windows/MSVC (happened once with an OpenJPEG-adjacent decode-region issue during this project's development — see `docs/BUILD_WINDOWS.md`) | Pin vcpkg to an earlier known-good commit, or build the affected library from its own upstream source and swap the DLL in |
