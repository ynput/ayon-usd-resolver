# [AYON](https://ynput.io/ayon/) USD Resolver

## Introduction

The [AYON](https://ynput.io/ayon/) USD Resolver is [an asset resolver plugin](https://openusd.org/release/api/ar_page_front.html#ar_uri_resolvers) for [Pixar's USD](https://openusd.org). It's designed to turn URIs with the `ayon://` or `ayon+entity://` formats into local file paths.

This resolver uses local caching and connects with the AYON Server to handle AYON compatible entity URIs through the [AyonCppApi]((https://github.com/ynput/ayon-cpp-api/)).


> [!IMPORTANT]  
> This repository is a _development_ repository and uses Git Submodules. Make sure to use the correct `git clone` commands accordingly.  

> [!IMPORTANT]  
> The [AYON](https://ynput.io/ayon/) USD Resolver is an [AR2.0](https://openusd.org/release/wp_ar2.html) resolver and will not support packages that only support AR1.0 interface.

> [!NOTE]  
> Building and testing is now done with Houdini 19.5 and Houdini 20. More packages will be available soon. To build against the "standalone" USD framework you need to either comment `include(BuildPlugins/${SelectedCompilePlugin}.cmake)` line in `CMakeLists.txt` or build you own build plugin in `BuildPlugins`.


## Required:

### Requirements:
- C++ Compiler
- CMake
- USD Framework or package with USD headers and libraries


### Tested Platforms:
- Alma Linux 9
	- Hou 19.5.805
	- Hou 19.5.900
	- Hou 20.0.590
	- Hou 20.0.630
- Windows 10
	- Hou 19.5.805
	- Hou 19.5.900
	- Hou 20.0.590
	- Hou 20.0.630
	- USD 24.03
 
### Download the repo and its submodules:  
```sh
git clone --recurse-submodules https://github.com/ynput/ayon-usd-resolver.git
git submodule update --init --recursive
```

## Pre-build / Manual build
In the future, we'll have pre-built versions of the resolver ready for you. But remember, these are the same ones our tests use, so they might not match the exact software or platform you need. Also, these prebuilt resolvers may not be up-to-date with the latest software releases.

### Pre-build
TBA - Pre-builds are not available now. They will be shipped with [AYON USD Addon](https://github.com/ynput/ayon-usd)
  
### Building

#### Core concepts 
We currently support Houdini for building Resolvers. Support for other software and stand-alone configurations is coming soon.

Building a Resolver involves using _.sh_ (Linux) and _.bat_ (Windows) shell scripts. The Linux script has more features because we mainly develop Resolvers on Linux, so `build.sh` includes additional functions. 


#### Linux Build Steps for Houdini:
First, set some variables in the `build.sh` script:

| Variable | Description |
| --- | --- |
| `HOU_VER` | This is your Houdini version. |
| `COMPILEPLUGIN` | In the root of the repo, you can find a folder called `BuildPlugins`. In this folder, there are `.cmake` scripts that we call _BuildPlugins_. You will have to set this variable to the path + name of this build plugin as a relative path starting from the `BuildPlugins`, e.g. `HouLinux/LinuxPy310Houdini20` |
| `INSTALLNAME` _(optional)_ | This is an optional variable that allows you to overwrite the resolver's folder name. |
| `HOUDINI_INSTALL_DIR` _(optional)_ | This overrides the install directory for Houdini. If you don't specify this, the script will think you installed Houdini in `opt/` under the name `hfs`. |

Run `build.sh Clean` / `Clean` to delete and re-create the build and Resolvers folder for a clean build.

Your resolver is compiled and will be under Resolvers + BuildPlugin path, e.g. `Resolvers/HouLinux/LinuxPy310Houdini20`


#### Windows Build Steps for Houdini:
Again, set few variables in the `build.bat` script:

| Variable | Description |
| --- | --- |
| `HFS` | Houdini install directory e.g `C:\Program Files\Side Effects Software\Houdini 20.0.590` |
| `COMPILEPLUGIN` | In the root of the repo, you can find a folder called `BuildPlugins`. In this folder, there are `.cmake` scripts that we call _BuildPlugins_. You will have to set this variable to the path + name of this build plugin as a relative path starting from the `BuildPlugins`, e.g. `HouWin/WindowsPy310Houdini20` |

Run `build.bat` and your resolver is compiled in `Resolvers` + BuildPluing Path. e.g. `Resolvers/HouLinux/LinuxPy310Houdini20`

### How to get the resolver working with Houdini and [AYON](https://ynput.io/ayon/)

#### General
The Resolver needs some environment variables to work, namely: 

| Variable | Description |
| --- | --- |
| `USD_ASSET_RESOLVER` | Define for Usd what resolver to use and where to find it (this will not overwrite the default resolver as a fallback). |
| `TF_DEBUG` | Defines what USD debug messages will be printed[^1]. |
| `LD_LIBRARY_PATH` | Defines where the C++ dynamic library files can be found for the resolver. |
| `PXR_PLUGINPATH_NAME` | Define where USD plug-ins are found - _it might look like you're supposed to place the AyonUsdResolver name in here, but you're actually putting the path to the `PluginInfo.json` folder into this variable._ |  

| `PYTHONPATH` | Used to include the resolver's Python wrapper functions to Python. |
| `AYONLOGGERLOGLVL` | Sets the logging level for the AyonCppApi - `INFO`, `ERROR`, `WARN`, `CRITICAL`, `OFF` |
| `AYONLOGGERFILELOGGING` | Enables or disables file logging in AyonCppApi - `OFF`, `ON`
| `AYONLOGGERFILEPOS` | Sets a filepath for the AyonCppApi logging - `/path/to` or `./relPath` |

 
Inside AYON, you can use the Environment Field of your software version to define what resolver you want to use. Here is an example of how that might look:

 ```json
{
    "AYONUSDRESOLVER_ROOT": "/path/to/ayon-usd-resolver/Resolvers/{BuildPlugin path + name}",
    "USD_ASSET_RESOLVER": "{AYONUSDRESOLVER_ROOT}",
    "TF_DEBUG": "",
    "LD_LIBRARY_PATH": [
        "{AYONUSDRESOLVER_ROOT}/ayonUsdResolver/lib",
        "{LD_LIBRARY_PATH}"
    ],
    "PXR_PLUGINPATH_NAME": [
        "{AYONUSDRESOLVER_ROOT}/ayonUsdResolver/resources",
        "{PXR_PLUGINPATH_NAME}"
    ],
    "PYTHONPATH": [
        "{PYTHONPATH}",
        "{AYONUSDRESOLVER_ROOT}/ayonUsdResolver/lib/python"
    ],
    "AYONLOGGERLOGLVL": "WARN",
    "AYONLOGGERFILELOGGING": "ON",
    "AYONLOGGERFILEPOS": "LoggingFiles"
}
```

## Developer Information

### Resolver Behavior:

On USD Init:  
- AyonUsdResolver library will be loaded, and a `ResolverContextCache` and a globally accessible [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) will be created

When a USD file is opened:
- Resolver Context is created.

	- very resolver context has a [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) that will point to the global `ResolverContextCache`

When a USD [AssetIdentifier](https://openusd.org/release/glossary.html#usdglossary-assetinfo) is found.
- `_Resolve()` gets called with the data between the [@](https://openusd.org/release/glossary.html#usdglossary-asset) symbols.
- `_Resolve()` checks if the path is an AYON URI path.
    - **Yes?** Then we get the current context (because in this resolver, the resolver context interacts with the [AyonCppApi](https://github.com/ynput/ayon-cpp-api/) and not the Resolver).  
        - We ask the ResolverContext to return the path to us, and the ResolverContext calls the `getAsset()` function in the `ResolverContextCache`.
        - The `ResolverContextCache` will then first check the PreCache and then the Responsible cache. If the `ResolverContextCache` finds the asset, it will be returned as a struct. If the `ResolverContextCache` does not find an asset, it will call the **AyonCppApi** and request the asset information from the server.
    - **No?** Then we check if the AssetIdentifier was already registered in the CommonCache, and if so, we will return the cached entry. If not, we will resolve the path against the operating system file system the same way that USD Default Resolver does it.


### Asset Identifier / Behavior:

The AssetIdentifier or AssetPath is always used by the resolver to convert an AYON path to a path on disk. The resolver needs some information in the path to figure out what asset you want.

1. `ayon:` is used in the `_resolve()` function to know whether your asset is an AYON asset or not (done via a string view comparison).
2. `//{ProjectName}/{path/to/ayon/folder}?product={FileName}` This is a classic AYON path that defines what Ayon folder you want, e.g., sequences/sh010, assets/characters/bob, etc.
3. `version=latest` version has multiple options:
    - `latest`: Will tell the resolver to use the latest version no matter what.
    - `hero`: This will tell the resolver to find the pinned hero version (you should know that you have the option to set up your AYON server without hero versions; in this case, the resolver will not be able to find your product version).
    - `v001` _(or whatever you put in your template)_: Will allow you to use a specific version of the product.

4. `representation=usd`: This part of the path is very important; it sets the file "extension" that the resolver will search for. You can use everything that you can upload to the server.

ll together, you will get an asset path like this. This asset path can be used inside of USD and will be resolved by the asset Resolver.

`ayon://{ProjectName}/{path/to/ayon/folder}?product={FileName}&version=latest&representation=usd`


[^1]: In the CPP files, you might find `TF_DEBUG().Msg();` and one
  of the two Enum Values `AYONUSDRESOLVER_RESOLVER` or `AYONUSDRESOLVER_RESOLVER_CONTEXT` these allow you to select what debug messages will be printed. If you want the resolver to be silent, then you can leave this value empty. It's best practice to keep it in your env variable setup, just in case.   

