# [AYON](https://ynput.io/ayon/) USD Resolver

A [Pixar's USD](https://openusd.org) [ArResolver Usd Resolver Plugin](https://openusd.org/release/api/ar_page_front.html#ar_uri_resolvers) for [AYON](https://ynput.io/ayon/).

## Features

- Resolves URIs with the schema `ayon://` or `ayon+entity://` into local file paths.
```
ayon://{project}/{path/to/ayon/folder}?product={product}&version={version}&representation={representation}
ayon+entity://{project}/{path/to/ayon/folder}?product={product}&version={version}&representation={representation}
```
- Local caching (see "Developer Information" for more details)
- Resolves the [AYON](https://ynput.io/ayon/) compatible entity URIs against the AYON-Server using the [AyonCppApi](https://github.com/ynput/ayon-cpp-api/).

> [!IMPORTANT]  
> This repository is a _development_ repository and uses Git Submodules. Make sure to use the correct `git clone` commands accordingly.  

> [!IMPORTANT]  
> The [AYON](https://ynput.io/ayon/) USD Resolver is a [AR2.0](https://openusd.org/release/wp_ar2.html) resolver and will not support packages that only support AR1.0

> [!NOTE]  
> Builds and tests are currently done against Houdini 19.5 and Houdini 20. Other packages will follow shortly.

## Required:

- C++ Compiler
- Cmake
- GitHub public key setup (this is because the submodules are linked via git@)
- Houdini Install

## Tested Platforms

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
 
## Download the repo and its submodules:  
```
git clone --recurse-submodules git@github.com:ynput/ayon-usd-resolver.git
git submodule update --init --recursive
```


## Building

### Prebuilt binaries

- Prebuilt binaries iof the resolver will be provided in the future.
- Do note however that those builds are prebuilt binaries used for our testing created, so you might not find the software or OS version you want.  
- It's also to be expected that resolver prebuilt binaries will lag behind compatibility to new DCC software releases.  
  
### Compiling yourself

## Core concepts

1. Currently, we only support Houdini for building Resolvers. (Other software Packages and stand-alone setups will follow soon)
2. Currently, building the Resolver centers around a `build.sh` (Linux) or `build.bat` (Windows) script. 

> [!NOTE]
> The Linux build script is more elaborate than the Windows script because resolver development is currently done on Linux, so the `build.sh` carries extra functionality around. 


### Linux Build Steps

- First, you must set a few variables in the `build.sh` script. (they are all greyed out)

#### Variables:
- `HOU_VER` = Set this to the number off your Houdini version.
- `COMPILEPLUGIN` = In the Reop-root, you find a folder called `BuildPlugins`. In this folder, there are .cmake scripts that we call BuildPlugins. You will have to set this variable to the path + name off this build plugin as a relative path
starting from the `BuildPlugins` e.g. `HouLinux/LinuxPy310Houdini20`
- `INSTALLNAME`{Optional} = This is an optional variable that allows you to overwrite how the folder for the resolver will be named. 
- `HOUDINI_INSTALL_DIR`{Optional} = this is an overwrite for the install directory off Houdini. If you don't set this, the script will assume that you installed Houdini in `opt/` with the base name of `hfs`

#### Next Steps {in the Terminal}:
- Run `build.sh Clean` / `Clean` = Will delete and recreate the build and Resolvers folder for a clean build setup. 
- Your resolver is compiled and will be under Resolvers + Build Plugin Path. e.g. `Resolvers/HouLinux/LinuxPy310Houdini20`


### Windows Build Steps

- First, you will have to set a few variables in the `build.bat` script.

#### Variables:
- `HFS` = this will be the Houdini install directory e.g `C:\Program Files\Side Effects Software\Houdini 20.0.590`
- `COMPILEPLUGIN` = In the Reop-root, you find a folder called `BuildPlugins`. In this folder, there are .cmake scripts that we call BuildPlugins. You will have to set this variable to the path + name off this build plugin as a relative path
starting from the `BuildPlugins`, e.g. `HouWin/WindowsPy310Houdini20`

#### Next Steps {in the Terminal}:
- Run `build.bat`.
- Your resolver is compiled and will be under Resolvers +BuildPluing Path. e.g. `Resolvers/HouLinux/LinuxPy310Houdini20`


## Using the Resolver

How to get the resolver working with Houdini and [AYON](https://ynput.io/ayon/)

### Environment Variables

The Resolver needs a few Env variables to work, namely:  

**USD_ASSET_RESOLVER**:  
- Used to tell USD what resolver to use and where to find it (this will not overwrite the default resolver fallback).

**TF_DEBUG**:  
- Defines what Debug messages will be printed.  
 	- In the CPP files, you might find TF_DEBUG().Msg();  and one of the two Enum Values `AYONUSDRESOLVER_RESOLVER` or `AYONUSDRESOLVER_RESOLVER_CONTEXT` these allow you to select what debug messages will be printed.  
   	- If you want the resolver to be silent, then you can leave this value empty. It's best practice to keep it in your env variable setup, just in case.   

**LD_LIBRARY_PATH**:  
- Describes where the C++ dynamic library files can be found for the resolver. 

**PXR_PLUGINPATH_NAME**:  
- USD variable to define locations of USD plug-ins.
    - It may look like you're supposed to specify the `AyonUsdResolver` name, but you're actually putting the full path to the `PluginInfo.json` folder into this variable.  

**PYTHONPATH**:
- Required to access the `AyonUsdResolver` python library, which are the Python bindings to the resolver context.

**AYONLOGGERLOGLVL**:  
- Set the log level for the CppApi.  
	- `INFO,ERROR,WARN,CRITICAL,OFF`
  	  
**AYONLOGGERFILELOGGING**:  
- Enable or disable file logging in CppApi.  
	- `OFF,ON`
  	  
**AYONLOGGERFILEPOS**:  
-  Set a logging filepath for the CppApi logs.  
	- `/path/to/log.txt` (this may be a full path or relative path)  
 
### Settings up Ayon USD Resolver in AYON Settings

Inside Ayon, you can use the **Environment** settings field of your application version to define what resolver you want to use. 
Here is an example of what that might look like:
 
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


# Developer Information

## Resolver Behavior:

On USD initialization:  
- `AyonUsdResolver` library will be loaded, and a `ResolverContextCache` and a globally accessible [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) will be created.

When a USD file is opened:
- A Resolver Context is Created.
	- Every resolver context has a [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) that will point to the global `ResolverContextCache`.

When a USD [AssetIdentifier](https://openusd.org/release/glossary.html#usdglossary-assetinfo) is found.
- `_Resolve()` gets called with the data between the [@](https://openusd.org/release/glossary.html#usdglossary-asset) symbols.
- `_Resolve()` checks if the path is an AYON URI path.
    - Yes?: Then we get the current context (because in this resolver, the resolver context interacts with the [AyonCppApi](https://github.com/ynput/ayon-cpp-api/) and not the Resolver).  
        - We ask the ResolverContext to return the path to us, and the ResolverContext calls the `getAsset()` function in the `ResolverContextCache`.
        - The `ResolverContextCache` will then first check the PreCache and then the Responsible cache. If the `ResolverContextCache` finds the asset, it will be returned as a struct. If the `ResolverContextCache` does not find an asset, it will call the `AyonCppApi` and request the asset information from the server.
    - No?: Then we check if the Asset Identifier was already registered in the CommonCache, and if so, we will return the cached entry. If not, we will resolve the path against the operating system file system the same way that USD Default Resolver does.


## Asset Identifier / Behavior:

The AssetIdentifier or AssetPath is always used by the resolver to convert an AYON path to a path on disk. The resolver needs some information in the path to figure out what asset you want.
1. `ayon:` is used in the `_resolve()`  function to know whether your asset is an AYON asset or not (done via a string view comparison).
2. `//{project_name}/{path/to/ayon/folder}?product={product_name}` This is a classic AYON path that defines what Ayon folder you want, e.g., sequences/sh010, assets/characters/bob, etc.
3. `version=latest` version has multiple options:
    - `latest`: Will tell the resolver to use the latest version no matter what.
    - `hero`: This will tell the resolver to find the pinned hero version (you should know that you have the option to set up your AYON server without hero versions; in this case, the resolver will not be able to find your product version).
    - `v001`  (Or whatever you put into your template): Will allow you to use a specific version of the product.
4. `representation=usd` This part of the path is very important; it sets the file "extension" that the resolver will search for. You can use everything that you can upload to the server.

All together, you will get an asset path like this. This asset path can be used inside of USD and will be resolved by the asset Resolver.
`ayon://{project_name}/{path/to/ayon/folder}?product={product_name}&version=latest&representation=usd`
