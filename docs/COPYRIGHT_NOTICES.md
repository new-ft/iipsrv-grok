# Copyright notices in this repository

This note explains how copyright notices are handled in `new-ft/iipsrv-grok`.

## Summary

**Existing copyright notices must not be replaced or removed.** This
repository is a modified version of iipsrv, which is licensed under the
GNU AGPLv3. The AGPLv3 requires that existing copyright and licence notices
are kept intact in copies and modified versions. Changing the names in the
headers would breach the licence and misattribute authorship.

## What is in the repository

| Area | Notice | Action |
|------|--------|--------|
| `src/*` (iipsrv) | `Copyright (C) <years> Ruven Pillay` (`KakaduImage.*`: `IIPImage`) | Keep unchanged |
| `windows/Time.cc`, `windows/Time.h` | `Copyright (C) <years> Ruven Pillay` | Keep unchanged |
| `src/Cache.h` | Ruven Pillay and Patrick Audley | Keep unchanged |
| `fcgi/` | Open Market, Inc.; Tux the Linux Penguin; Rob Saccoccio and Chelsea Networks | Keep unchanged |
| `m4/*.m4` | Various autoconf-archive authors | Keep unchanged |
| `COPYING.ijg` | Thomas G. Lane, Guido Vollbeding (IJG) | Keep unchanged |
| `autogen.sh`, `INSTALL`, `scripts/openrc/iipsrv.initd`, `docker/README` | Their respective authors / organisations | Keep unchanged |
| `third_party/grok` | Grok authors (see `third_party/grok/LICENSE`) | Keep unchanged |

Lines that mention `COPYRIGHT` in `Environment.h`, `Main.cc`, `FIF.cc`, the
man page and the config scripts are about the `COPYRIGHT` environment
variable (a rights statement for served images). They are not licence
notices and need no change.

## Our modifications

For files we modified or created, a line for our own changes sits directly
**below** the existing notice:

```
Copyright (C) 2006-2026 Ruven Pillay
Modifications Copyright (C) 2026 World Archives Holdings LLC
```

Files carrying this line: `src/Main.cc`, `src/Environment.h`, `src/FIF.cc`,
`src/DeepZoom.cc`, `src/IIIF.cc`, `src/TIL.cc`, `src/IIPResponse.cc/.h`,
`src/OpenJPEGImage.cc`, `src/KakaduImage.cc`, `src/GrokImage.cc/.h` and
`src/GrokMTImage.cc/.h`. The full list of changes is in
`CHANGES_FROM_UPSTREAM.md`. Existing notices are unchanged.

## Remaining decision for the client

Legal sign-off on the approach above. This note is a technical description,
not legal advice.
