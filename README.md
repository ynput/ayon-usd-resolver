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
> There are some protocol compatible alternatives to memcached, like [memc-rs](https://memc.rs), etc.

Memcached can run as a service on a local machine or in a Docker container. You can even setup multiple servers and utilize client hashing feature that distributes keys across a cluster of servers using hashing algorithms. This ensures that adding or removing servers causes only a minimal shift in key mapping, preventing massive cache misses. Note that this depends on the used memcached client features.

### How it works
If memcached is enabled and the asset resolver cannot find the path in the context cache, it will try memcache configured by environment variable `AYON_MEMCACHED_SERVERS` (comma separated `host:port` list). Only if this misses, it will get the path from AYON server (and then store it in memcache for further use).

Resolver doesn't deal with pre-seeding memcache with entries. AYON USD addon can do that, but since memcached is open, any mechanism can be used.

### How to build with memcached support
You only need to add `--with-memcached` argument to build command. You need to have `libmemcached` available.

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
On Linux and macOS the build system uses [pkg-config](https://en.wikipedia.org/wiki/Pkg-config) along with the standard installation paths. With Linux, use your packager to install `libmemcached-devel` or similar. On macOS, Homebrew is your friend - `brew install libmemcached`.


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
