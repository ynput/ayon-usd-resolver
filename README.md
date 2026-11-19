# [AYON](https://ynput.io/ayon/) USD Resolver

## Introduction


The [AYON](https://ynput.io/ayon/) USD Resolver is
[an asset resolver plugin](https://openusd.org/release/api/ar_page_front.html#ar_uri_resolvers)
for [Pixar's USD](https://openusd.org). It's designed to turn URIs with the
`ayon://` or `ayon+entity://` formats into local file paths.

This resolver uses local caching and connects with the AYON Server to handle
AYON compatible entity URIs through the
[AyonCppApi](<(https://github.com/ynput/ayon-cpp-api/)>).

> [!IMPORTANT]\
> This repository uses Git Submodules. Make sure to use the correct `git clone`
> commands accordingly.\
> `git clone --recurse-submodules https://github.com/ynput/ayon-usd-resolver.git`\
> `git submodule update --init --recursive`

> [!IMPORTANT]\
> The [AYON](https://ynput.io/ayon/) USD Resolver is an
> [AR2.0](https://openusd.org/release/wp_ar2.html) resolver and will not support
> AR1.0 resolution. Make sure that your software package is compatible with the
> AR2.0 standard or use the **defaults** we provide in the
> [AYON Usd Addon](https://github.com/ynput/ayon-usd)

> **NOTE**\
> **Admin** and **Dev** docs can be found under `/Docs/Ayon_Docs/`

## Memcached support
For faster access the resolver can use [memcached](https://docs.memcached.org/).

### What is memcached
Memcached is a free, open-source distributed memory caching system. It speeds up dynamic, database-driven websites and applications by storing frequently accessed data and objects directly in RAM, reducing the number of times a system must query slower external data sources like a database or API.

> [!NOTE]
> There are some protocol compatible alternatives to memcached, like [Dragonfly](https://www.dragonflydb.io/). The resolver uses the text protocol, so the server has to support it. [memc-rs](https://memc.rs), for example, speaks only the binary protocol and does not work.

Memcached can run as a service on a local machine or in a Docker container. You can even setup multiple servers: the client distributes keys across them by hashing. The resolver uses libmemcached's default distribution, so changing the server list moves most keys to another server, and those URIs go to AYON until the cache fills again. Use the same list, in the same order, on every machine.

### How it works
If memcached is enabled and the asset resolver cannot find the path in the context cache, it will try memcached configured by environment variable `AYON_MEMCACHED_SERVERS` (comma separated `host:port` list). Only if this query misses, it will get the path from AYON server (and then store it in memcaches for further use as a rootless path).

Resolver doesn't deal with pre-seeding memcached with entries. It will store all path there so just by using
the cache gets populated over time.

To use memcached support, set following environment variables

`AYON_MEMCACHED_SERVERS` - comma separated list of `host:port` defined servers. When more servers are defined, keys are split between them, so they are not fallbacks for each other. If any server cannot be reached when the process starts, memcached stays off for that whole process, even with the other servers up: a warning is logged and lookups go to AYON. If a server is lost during a session, paths stay correct and more lookups go to AYON.

`AYON_MEMCACHED_ENABLED` - set to `true` to enable memcached support.

`AYON_MEMCACHED_TIMEOUT_MS` - control connection timeout - default value is 1000ms.

`AYON_MEMCACHED_EXPIRE_SECONDS` - lifetime of cached entries for immutable versions (integer versions such as `v004` or `4`) in seconds. `0` (default) = never expire. Memcached treats relative lifetimes above 30 days (2592000 s) as absolute timestamps, so larger values are clamped to 2592000 with a warning. Invalid values fall back to the default with a warning.

`AYON_MEMCACHED_HERO_EXPIRE_SECONDS` - same, but only for URIs whose `version` query parameter is `hero` (a hero republish re-points the hero version in place, so its path can change). `3600` (default) = one hour. `-1` = do not use memcached for them at all, `0` = never expire, `>0` = seconds (clamped as above).

`AYON_MEMCACHED_LATEST_EXPIRE_SECONDS` - same, but only for URIs whose `version` query parameter is `latest` or `latestdone` (these resolve to a path that changes when a new version is published). `-1` (default) = do not use memcached for them at all (neither read nor write), so they are always resolved live from AYON. `0` = never expire, `>0` = seconds (clamped as above). Memcached is not consulted for these URIs even if older resolver builds stored them. The in-process cache is unaffected: a running session keeps its answer until the cache is cleared.

| `version` value | Class | Setting | Default |
|---|---|---|---|
| `latest`, `latestdone` | latest | `AYON_MEMCACHED_LATEST_EXPIRE_SECONDS` | `-1` (bypass) |
| `hero` | hero | `AYON_MEMCACHED_HERO_EXPIRE_SECONDS` | `3600` |
| an integer (`v004`, `4`) | immutable | `AYON_MEMCACHED_EXPIRE_SECONDS` | `0` (never) |
| missing, `*` or any other value | latest | `AYON_MEMCACHED_LATEST_EXPIRE_SECONDS` | `-1` (bypass) |

Values are matched like the server matches them: trimmed, case-insensitively, one leading `v` ignored. Without an integer, hero or latest value the server can return any version, so such URIs are never treated as immutable. If a URI carries several `version` parameters, latest wins over hero, hero over immutable.

`ClearCache()` (and the Houdini "clear resolver cache" actions) clears the in-process caches and deletes this session's hero/latest keys from the regional memcached (not for a class set to `-1`, which never wrote them). It never flushes memcached (shared by all artists), and integer-version entries are untouched.

`AYON_MEMCACHED_MISS_EXPIRE_SECONDS` - negative cache for failed lookups (e.g. a placeholder product with nothing published yet). When AYON has no path for an AYON URI, an empty value is stored in memcached under the same key for this many seconds (builds without miss markers read it as a plain miss); other processes that read the marker treat the URI as unresolved without calling AYON. `60` (default), `-1` = do not cache misses, `>0` = seconds (clamped to 2592000 with a warning). `0` is refused (default + warning): a miss that never expires would hide a first publish if the invalidation watcher is not running. Misses follow the class rule above: URIs whose class bypasses memcached (e.g. `latest` by default) never get a marker.

The marker is never returned as a path: a known miss yields exactly what an uncached failed resolve yields, and is not stored in the in-process caches. A process trusts a marker it saw for up to 5 seconds without reading memcached again, so a stage with many dangling references does not make a round trip per lookup. A publish can stay hidden from that process for that long after another process deletes the marker or the marker expires, because the 5 seconds count from when the marker was read (the window is shorter only when this process's own miss TTL is shorter). Transport errors are not misses and write no marker. The invalidation watcher deletes a URI's miss marker when its first version is created; the TTL is the fallback if the watcher is not running. `ClearCache()` also deletes the miss markers this process saw, and `DeleteFromCache(uri)` deletes that URI's marker. A marker is deleted only while memcached still holds it, so a path stored in the meantime (e.g. after a publish) is kept.

### Stage prewarm
When a stage is opened, the resolver first walks its layers and resolves the AYON URIs it finds in batched requests, instead of one request per URI during composition. This runs once per root asset path in a process and is on by default, with or without memcached.

`AYON_RESOLVER_NO_PREWARM` - set to anything except `0` or `false` to turn prewarm off. `AYON_RESOLVER_PREWARM` set to `0` or `false` does the same.

Prewarm opens each layer it walks and then drops it, so composition opens that layer again. Stages with many AYON references open faster, but a large text layer can take about twice as long to open. In one test, a 7.5 MB `.usda` sublayer took 0.83 s instead of 0.37 s. `.usdc` was not measured.

### How to build with memcached support
You need to have `libmemcached` available. Memcached support is built whenever the build finds it, otherwise the resolver is built without it. The `--with-memcached` argument of `build_resolver.py` does not change this: it only adds a warning when `libmemcached` is not found.

### Windows
On windows, the best way is to use [vcpkg](https://github.com/microsoft/vcpkg).

```powershell
git clone https://github.com/microsoft/vcpkg C:\dev\vcpkg
$env:VCPKG_ROOT="C:\dev\vcpkg"
$env:PATH="$env:VCPKG_ROOT;$env:PATH"
C:\dev\vcpkg\bootstrap-vcpkg.bat
# install libmemcached
vcpkg install libmemcached-awesome:x64-windows-static-md
```
The build should find `libmemcached` and proceed with the build. Without vcpkg the build system will check common installation paths.

## Linux and  macOS
On Linux and macOS the build system uses [pkg-config](https://en.wikipedia.org/wiki/Pkg-config) along with the standard installation paths. With Linux, use your packager to install `libmemcached-devel`, `libmemcached-awesome-devel` or similar. On macOS, Homebrew is your friend - `brew install libmemcached`.

To get newer version, it is very easy to [build it and
install it](https://github.com/awesomized/libmemcached#from-source) as usual and build system will find it.

# Repository Docs


### Requirements:

- C++ Compiler
- Cmake
- Target DCC / SDK installed
- python3 development files (Optional when building Against AYON Usd)

### Python Environment (uv)

This repository uses [uv](https://docs.astral.sh/uv/) for Python dependency
management.

Install dependencies:

```
uv sync --group dev
```

Run project tooling with uv:

```
uv run pre-commit run --all-files
uv run pytest
```


### Tested Platforms:

- Rocky 9 Linux
  - Hou 21.0.631
  - Hou 21.0.512
  - Hou 20.5.892
  - Hou 20.5.654
  - Hou 20.0.1358
  - Hou 20.0.896
  - Hou 19.5.805
  - Hou 19.5.773
  - Maya 2026 (UsdAddon_0.25.5)
  - Maya 2025
  - Maya 2024

- Windows 11
  - Hou 22.0.368
  - Hou 21.0.512
  - Hou 20.5.370
  - Hou 20.5.332
  - Hou 20.0.705
  - Hou 20.0.688
  - Hou 19.5.805
  - Hou 19.5.976
  - Maya 2026 (UsdAddon_0.25.5)
  - Maya 2025
  - Maya 2024


#### Run build using python script
Run build (Houdini):
```
python build_resolver.py --dcc houdini --dcc-root <houdini/installation/path> --clean --zip
```

Run build (Maya):
```
python build_resolver.py --dcc maya --dcc-root <maya/installation/path> --maya-devkit <maya/devkit/path> --maya-usd-root <maya/usd/root/path> --jobs 4 --clean
```

or see the help:
```
python build_resolver.py --help
```

#### Run build using cmake only
Run build (Houdini):
```
cmake -S . -B <build/dir/path> -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<install/dir/path> -DBUILD_TARGET=houdini -DUSE_OPENSSL3=OFF -DUSD_ROOT=<houdini/root/dir> -DCMAKE_PREFIX_PATH=<houdini/cmake/path> -DPYTHON_EXECUTABLE=<houdini/python/path>
```
```
cmake --build <build/dir/path> --target install -j 4
```

Run build (Maya):
```
cmake -S . -B "/build/dir/path" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<install/dir/path> -DBUILD_TARGET=maya -DUSE_OPENSSL3=ON -DMAYA_ROOT=<maya/root/path> -DMAYA_USD_DEVKIT_PATH=<maya/devkit/path> -DUSD_ROOT=<maya/usd/path> -DPYTHON_EXECUTABLE=<maya/python/path>
```
```
cmake --build <build/dir/path> --target install -j 4
```

## Download the repo and its submodules:

    ```
    git clone --recurse-submodules https://github.com/ynput/ayon-usd-resolver.git

    git submodule update --init --recursive
    ```

## DoxyGen Docs

Can be found on the connected GH Page
[Docs GH Page](https://ynput.github.io/ayon-usd-resolver/)

## Pre-build / Self Compiled

- AYON provides some pre-built versions of the Resolver at https://lake.ayon.cloud. Access credentials can be provided upon request.

- Those versions are the pre-built binaries that our tests created, so you
  might not find your software/platform combination.
- It's also to be expected that resolver builds are behind new software
  releases.

## Prebuild

- Prebuilts aren't available as of right now.

## Self Compiled

- See [Doxygen Docs](https://ynput.github.io/ayon-usd-resolver/md_md_Getting_Started.html) Getting Started Page
