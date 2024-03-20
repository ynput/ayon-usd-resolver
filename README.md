# [AYON](https://ynput.io/ayon/) USD Resolver  
The [AYON](https://ynput.io/ayon/) USD Resolver is a [Pixar's USD](https://openusd.org) / [ArResolver Usd Resolver Plugin](https://openusd.org/release/api/ar_page_front.html#ar_uri_resolvers)  
its build to resolve URIs with the schema `ayon://` or `ayon+entity://` into local file paths.   
The resolver implements local caching and resolves the [AYON](https://ynput.io/ayon/) compatible entity URIs against the AYON-Server using the [AyonCppApi](https://github.com/ynput/ayon-cpp-api/).   

> [!IMPORTANT]  
> This repository is a _development_ repository and uses Git Submodules. Make sure to use the correct `git clone` commands accordingly.  

> [!IMPORTANT]  
> The [AYON](https://ynput.io/ayon/) USD Resolver is a [AR2.0](https://openusd.org/release/wp_ar2.html) resolver and will not suport packages that only suport AR1.0

> [!NOTE]  
> Building and testing are currently done against Houdini 19.5 and Houdini 20. Other packages will follow shortly.


## Required:
- C++ Compiler
- Cmake
- GitHub public key setup (this is because the submodules are linked via git@)
- Houdini Install

## Tested Platforms:
- Alma Linux 9
	- Hou 19.5.805
	- Hou 19.5.900
	- Hou 20.0.590
	- Hou 20.0.630
    - AyonUsd23_5_py39 (System Python install)
- Windows 10
	- Hou 19.5.805
	- Hou 19.5.900
	- Hou 20.0.590
	- Hou 20.0.630
    - AyonUsd23_5_py39 (Pyenv-Win)
 
## Download the repo and its submodules:  
    ```
    git clone --recurse-submodules git@github.com:ynput/ayon-usd-resolver.git
    git submodule update --init --recursive
    ```

# -------------------------- Prebuild / Self Compiled  --------------------------
- We will deliver some prebuild versoins off the resolver in the future.
- but it's essential to know that those versions will be the prebuilt binaries that our tests created, so you might not find the software or OS version you want.  
- It's also to be expected that resolver prebuilds are behind new software releases.  

# ---------------------------------- Prebuild ----------------------------------
- Prebuilds aren't available as of right now.
  
# ------------------------------- Self Compiled -------------------------------

## Core concepts 
1. currently, we only support Houdini for building Resolvers. (Other software Packages and stand-alone setups will follow soon)
2. Currently, building the Resolver centres around a build script .sh(Linux) .bat(windows). The Linux build script is more elaborate than the Windows script because resolver development is currently done on Linux, so the build.sh carries extra functionality around. 

### Linux Build Steps:
- First, you must set a few variables in the `build.sh` script. (they are all greyed out)
#### Varlibes:
- `HOU_VER` = Set this to the number off your Houdini version.
- `COMPILEPLUGIN` = In the Reop-root, you find a folder called `BuildPlugins`. In this folder, there are .cmake scripts that we call BuildPlugins. You will have to set this variable to the path + name off this build plugin as a relative path
starting from the `BuildPlugins` e.g. `HouLinux/LinuxPy310Houdini20`
- `INSTALLNAME`{Optonal} = This is an optional variable that allows you to overwrite how the folder for the resolver will be named. 
- `HOUDINI_INSTALL_DIR`{Optonal} = this is an overwrite for the instal directory off Houdini. If you don't set this, the script will assume that you installed Houdini in `opt/` with the base name of `hfs`

#### Next Steps {in the Terminal}:
- Run `build.sh Clean` / `Clean` = Will delete and recreate the build and Resolvers folder for a clean build setup. 
- Your resolver is compiled and will be under Resolvers +BuildPluing Path. e.g. `Resolvers/HouLinux/LinuxPy310Houdini20`


### Windows Build Steps:
- First, you will have to set a few variables in the `build.bat` script.
#### Varlibes:
- `HFS` = this will be the Houdini install directory e.g `C:\Program Files\Side Effects Software\Houdini 20.0.590`
- `COMPILEPLUGIN` = In the Reop-root, you find a folder called `BuildPlugins`. In this folder, there are .cmake scripts that we call BuildPlugins. You will have to set this variable to the path + name off this build plugin as a relative path
starting from the `BuildPlugins`, e.g. `HouWin/WindowsPy310Houdini20`

#### Next Steps {in the Terminal}:
- Run `build.bat`.
- Your resolver is compiled and will be under Resolvers +BuildPluing Path. e.g. `Resolvers/HouLinux/LinuxPy310Houdini20`


## How to get the resolver working with Houdini and [AYON](https://ynput.io/ayon/)
### General. 
The Resolver needs a few Env variables to work, namely:  

USD_ASSET_RESOLVER:  
- This variable tells Usd what resolver to use and where to find it (this will not overwrite the default resolver as a fallback).

TF_DEBUG:  
- This variable allows you to choose what Debug messages will be printed.  
 	- In the CPP files, you might find TF_DEBUG().Msg();  and one of the two Enum Values AYONUSDRESOLVER_RESOLVER or AYONUSDRESOLVER_RESOLVER_CONTEXT these allow you to select what debug messages will be printed.  
   	- If you want the resolver to be silent, then you can leave this value empty. It's best practice to keep it in your env variable setup, just in case.   

LD_LIBRARY_PATH:  
- it describes where the C++ dynamic library files can be found for the resolver. 

PXR_PLUGINPATH_NAME:  
- This is also a variable for Usd, and it might look like you're supposed to place the AyonUsdResolver name in here, but you're actually putting the path to the PluginInfo.json folder into this variable.  

PYTHONPATH:
- This is again a path for Usd that allows you to access the Python wrapper functions from the resolver inside Usd.

AYONLOGGERLOGLVL:  
- This Environment variable allows you to set the log level for the CppApi.  
	- INFO,ERROR,WARN,CRITICAL,OFF
  	  
AYONLOGGERFILELOGGING:  
- This Environment variable allows you to enable or disable file logging in CppApi.  
	- OFF,ON
  	  
AYONLOGGERFILEPOS:  
-  This Environment variable allows you to set a file path for the CppApi logging.  
	- /path/to or relPath  
 
 ### Inside Ayon, you can use the Environment Field of your software version to define what resolver you want to use. Here is an example of how that might look:
 ```
{
"USD_ASSET_RESOLVER":"/path/to/ayon-usd-resolver/Resolvers/{BuildPlugin path + name}",
"TF_DEBUG":"",
"LD_LIBRARY_PATH":"/path/to/ayon-usd-resolver/Resolvers/{BuildPlugin path + name}/ayonUsdResolver/lib:$LD_LIBRARY_PATH",
"PXR_PLUGINPATH_NAME":"/path/to/ayon-usd-resolver/Resolvers/{BuildPlugin path + name}/ayonUsdResolver/resources:$PXR_PLUGINPATH_NAME",
"PYTHONPATH":["{PYTHONPATH}","/path/to/ayon-usd-resolver/Resolvers/{BuildPlugin path + name}/ayonUsdResolver/lib/python"],
"AYONLOGGERLOGLVL":"WARN",
"AYONLOGGERFILELOGGING":"ON",
"AYONLOGGERFILEPOS":"LoggingFiles"
}
```


# ------------------------------ Developer Information ------------------------------

## Resolver Behavior:

On USD Init:  
- AyonUsdResolver library will be loaded, and a `ResolverContextCache` and a globally accessible [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) will be created

When a Usd file is opened
- Resolver Context is Created.
	- every resolver context has an [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) that will point to the global `ResolverContextCache`

USD [AssetIdentifier](https://openusd.org/release/glossary.html#usdglossary-assetinfo) is found.
- `_Resolve()` gets called with the data between the [@](https://openusd.org/release/glossary.html#usdglossary-asset) symbols.
- `_Resolve()` checks if the path is an AYON URI path.
    - Yes?: Then we get the current context (because in this resolver, the resolver context interacts with the [AyonCppApi](https://github.com/ynput/ayon-cpp-api/) and not the Resolver).  
        - We ask the ResolverContext to return the path to us, and the ResolverContext calls the `getAsset()` function in the `ResolverContextCache`.
        - The `ResolverContextCache` will then first check the PreCache and then the Responsible cache. If the ResolverContextCache finds the asset, it will be returned as a struct. If the ResolverContextCache does not find an asset, it will call the AyonCppApi and request the asset information from the server.
    - No?: Then we check if the AssetIdentifier was already registered in the CommonCache, and if so, we will return the cached entry. If not, we will resolve the path against the operating system file system the same way that USD Default Resolver Does it.


## Asset Identifier / Behavior:

The AssetIdentifier or AssetPath is always used by the resolver to convert an AYON path to a path on disk. The resolver needs some information in the path to figure out what asset you want.
1. `ayon:` is used in the `_resolve()`  function to know whether your asset is an AYON asset or not (done via a string view comparison).
2. `//{ProjectName}/{path/to/ayon/folder}?product={FileName}` This is a classic AYON path that defines what Ayon folder you want, e.g., sequences/sh010, assets/characters/bob, etc.
3. `version=latest` version has multiple options:
    - `latest`: Will tell the resolver to use the latest version no matter what.
    - `hero`: This will tell the resolver to find the pinned hero version (you should know that you have the option to set up your AYON server without hero versions; in this case, the resolver will not be able to find your product version).
    - `v001`  (Or whatever you put into your template): Will allow you to use a specific version of the product.
4. `representation=usd` This part of the path is very important; it sets the file "extension" that the resolver will search for. You can use everything that you can upload to the server.

All together, you will get an asset path like this. This asset path can be used inside of USD and will be resolved by the asset Resolver.
`ayon://{ProjectName}/{path/to/ayon/folder}?product={FileName}&version=latest&representation=usd`
