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

# Repository Docs


### Requirements:

- C++ Compiler
- Cmake
- Target DCC / SDK installed
- python3 development files (Optional when building Against AYON Usd)


### Tested Platforms:

- Alma Linux 9
  - Hou 19.5.805
  - Hou 19.5.900
  - Hou 20.0.590
  - Hou 20.0.630
  - Maya 2024.2(UsdAddon_0.25.0)
  - Unreal5.4
  - AyonUsd23_5_py39 (System Python install)

- Windows 10
  - Hou 19.5.805
  - Hou 19.5.900
  - Hou 20.0.590
  - Hou 20.0.630
  - AyonUsd23_5_py39 (Pyenv-Win)
  - Maya2024_2
  - Maya2025_2
  - Unreal5_4

## Download the repo and its submodules:

    ```
    git clone --recurse-submodules https://github.com/ynput/ayon-usd-resolver.git

    git submodule update --init --recursive
    ```

## DoxyGen Docs

Can be found on the connected GH Page
[Docs GH Page](https://ynput.github.io/ayon-usd-resolver/)

## Pre-build / Self Compiled

- AYON will provide some pre-built versions of the resolver in the future.

- Those versions will be the pre-built binaries that our tests created, so you
  might not find your software/platform combination.
- It's also to be expected that resolver builds are behind new software
  releases.

## Prebuild

- Prebuilts aren't available as of right now.

## Self Compiled

- See [Doxygen Docs](https://ynput.github.io/ayon-usd-resolver/md_md_Getting_Started.html) Getting Started Page
