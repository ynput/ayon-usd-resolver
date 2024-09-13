# [AYON](https://ynput.io/ayon/) USD Resolver

## Introduction

The [AYON](https://ynput.io/ayon/) USD Resolver is an
[AR2.0 AssetResolver Plugin](https://openusd.org/release/api/ar_page_front.html#ar_uri_resolvers)
for [Open USD](https://openusd.org) it has been designed to make Cross team,
Platfrom and software interoperability easy and accessable for all AYON users.
it is best used with our new Contribution workflow.

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
  - Maya 2024.2(UsdAddon_0.25.0)
  - Maya 2025
  - Unreal5.4
  - AyonUsd23_5_py39 (Pyenv-Win)

## Pre-build / Self Compiled

There are multiple Options to get your hands on the AyonUsdAsset resolver.

**[AyonUsdAddon](https://github.com/ynput/ayon-usd)**\
the AyonUsd addon will automatically download and setup a list of compiled
Resolvers.\
it is generally recommended to go with the AyonUsd addon if you don't want to
develop with the resolver as recompiling and setting up your own LakeFs setup
can be involved.

**Self Compiled**\
if you want to compile your own resolver because you want to implement a new
feature, fix a bug or add a new software to compile against.

## Self Compiled

> **Note**\
> Currently, we only support specific set of DCCs and their versions, and
> AyonUsd for building revolvers (other software packages and stand-alone setups
> will follow).

> **Note**\
> we updated the build startup script to be run via python to make cross
> platfrom development easyer.\
> in the future we intend to use AyonAutomator to provide build, test, developer
> and document stages with different build options.

## `build.py` variables

`--DEV`\
if you set this variable to one the CMake build script will be instructed to
compile in debug mode. It will also enable all Developer Macros.\
it is not advised to use this for production as it will impact the performance.

`--JTRACE`\
the resolver project includes an option to output a tracing file.\
this tracing file can be used with PerfettoUi in order to get detailed
information's about what functions where called while the resolver was running.\
this system works exclusively at runtime and will only show the functions that
where called no static testing is done.\
you should not use this in production as it will drastically impact the
performance.

`--Clean`\
CMake uses an internal cache wile its compiling. this helps speed up the compile
process but it can create issues if a variable changed that cmake did not
monitor.\
using Clean build will remove the compiled resolver files and the build
directory and call CMake clean first. this way we can ensure that the new
complie has a clean environment\
we advice to run a Clean compile when you want to release a new resolver or if
you run a CiCd process.

`--CompilePlugin`\
there are 2 options to select a compile plugin.

1. use this build script option.
2. use a Env variable.

In the end of the day this option will just set the Env variable for you.

`--extra_env_vars`\
this flag allows you to set key value pairs that will be setup as environment
variables. eg if your build plugin needs some Env variables you can set this via
this system its not mandatory tho.

## Compile Plugin's

we use compile plugins to allow compilation against different platforms and
software packages.\
using compile plugins allows us to have a small and efficient CMake setup while
also allowing build setups to be simple per Package instead of one big setup.

### How dose it work ?

**Folder Layout**\
in the root of this Reposetory you will find a folder called `BuildPlugins/`\
in this foulder you find a few subfoulders They are named via
`{AppName}{Platfrom_Name}`\
all those foulders hold BuildPlugins. every build plugin is for one specific
application varient. (if different variants of a software share the same
resolver one BuildPlugin for all of them is enough)

**What Dose a Build Plugin Doo ?**\
Every application implemented OpenUsd in a different way.\
Houdini for example has all the include files in one dir and all the bin files
in a separate one. But Maya has a zip file for some of the files. so in the end
a BuilPlugin will just need to setup a few Folder paths that we need in order to
allow the Build Process to find the right files and library's.

**How to select a build Plugin**\
Build plugins are selected by there path starting at `BuildPlugins/`\
so the path for `AyonUsd23_5_Py39_Linux` would be
`AyonUsdLinux/AyonUsd23_5_Py39_Linux` you should not include the .cmake
extension as the main CMake file will append it to the path.

**Build Plugin Requirements**\
Every build Plugin usually has some Env variables you need to set.\
For Houdini build Plugins you will need to set the root path to your Houdini
install.\
in the end of the day you will need to look into the BuildPlugin file to know
what the needed variables are in case they are not documented in Dev_Docs.

**The Build Command**\
a few things about the build script first.

1. you can start the build script from anny location and it will find its paths
   relative to its own loacaton so dont move the build script.
2. the build script compiles a single resolver, and generates a zip. you dont
   need the zip to work with the resolver locally but the Ynput LakeFs setup
   needs the zip to work correctly.

The command is usage as follows\
`python scripts/build.py [Flags]`\
most of the time you will use the `--CompilePlugin` and `--extra_env_vars` the
first allows you to select the Build Plugin to build with and the second one can
be useful to set the BuildPlugin related env vars\
Excample:\
`python scripts/build.py --CompilePlugin=AyonUsdLinux/AyonUsd23_5_Py39_Linux --extra_env_vars AyonUsdRoot={Path To UsdLib}`\
also remember that you need to use `--Clean` when you want to do a clean
compile. and when you change the build plugin you will need to run **Clean** so
that CMake delets the cached data for the BudilPlugin