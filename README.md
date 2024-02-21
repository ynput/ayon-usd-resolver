# AYON USD Resolver  
The AYON USD Resolver is a [Pixar's USD](https://openusd.org) [ArResolver Usd Resolver Plugin](https://openusd.org/release/api/ar_page_front.html#ar_uri_resolvers)  
that resolves URIs with the schema `ayon://` found in a USD file into local filepaths.  
The resolver implements local caching and resolves the AYON compatible entity URIs against the Ayon Server using the `AyonCppApi`.  

> [!IMPORTANT]  
> This repository is a _development_ repository and uses Git Submodules. Make sure to use the right `git clone` commands accordingly.  

> [!NOTE]  
Building and testing is currently done against Houdini 19 and Houdini 20. Other packages will follow shortly.


## Required:
- C++ Compiler
- Cmake
- GitHub public key setup (this is because the submodules are linked via git@)
- Houdini Install

## Tested Platforms:
- Alma Linux 9
- Windows 10

## Prebuild / Self Compiled

### Self Compiled:
1. Download the repo and its submodule:
    ```
    git clone --recurse-submodules git@github.com:ynput/ayon-usd-resolver.git
    git submodule update --init --recursive
    ```
   
### Windows:
- Set the Houdini install directory inside of the `build.bat` file.
- Run `build.bat`.
- Now your resolver is compiled.
- The `dist` folder will now contain the `ayonUsdResolver` folder (this is the compiled resolver).  
#### For testing
- There is a `launchHouWithResolver.bat`, point the last command to your Houdini install directory and execute the script inside of a shell. This will set up the environment variables needed for the Resolver and then start Houdini.
- Optional: In the `launchHouWithResolver.bat`, you have the option to disable `TF_DEBUG` by commenting out the line.

### Linux:
- Set the Houdini install directory inside of the `build.sh` file.
- Run `build.sh Clean`.
- Now your resolver is compiled.  
- The `dist` folder will now contain the `ayonUsdResolver` folder (this is the compiled resolver). ( ignore all foulders exept `ayonUsdResolver`, they are not important to you )  
#### For testing
- The `dist` folder will now contain the `ayonUsdResolver` folder (this is the compiled resolver).
- There is a `launchHouWithResolver.sh`, point the last command to your Houdini install directory and execute the script inside of a shell. This will set up the environment variables needed for the Resolver and then start Houdini.

## How to get the resolver working with Houdini and AYON
### General. 
- the Resolver needs a few Env varibles to work. Namingly
  USD_ASSET_RESOLVER
  	- this varible tells Usd what resolver to use and where to find it ( this will not overwrite the default resolver as a fallback )  
  TF_DEBUG
	- this varible allows you to choose what Debug massage will be printed.
 		- in the cpp files you might find TF_DEBUG().Msg(); and one off 2 Enum Values AYONUSDRESOLVER_RESOLVER or AYONUSDRESOLVER_RESOLVER_CONTEXT those allow you to select what debug massages will be printed.
   		- if you want the resolver to be silent, then you can just leave this value empty. its best practice to keep it in your env varible setup just in case. 
  LD_LIBRARY_PATH
	- this is allso an important path for Usd it describes where the c++ dynamic libary files can be found for the resolver. 
  PXR_PLUGINPATH_NAME
	- this is allso an var for Usd and it might look like your suposed to place the AyonUsdResolver name in here but your acctually puting the path to the PluginInfo.json foulder into ths varible 
  PYTHONPATH
	- this is yet again a path for Usd that allows you to acces the python wrapper functons from the resolver from inside off Usd
 
 ### Inside off Ayon you can use the Environment Field off your software versoin to define what resolver you want to use
 here is an excample how that might look 
 ```
{
"USD_ASSET_RESOLVER":"/path/to/ayon-usd-resolver/dist",
"TF_DEBUG":"",
"LD_LIBRARY_PATH":"/path/to/ayon-usd-resolver/dist/ayonUsdResolver/lib:$LD_LIBRARY_PATH",
"PXR_PLUGINPATH_NAME":"/path/to/ayon-usd-resolver/dist/ayonUsdResolver/resources:$PXR_PLUGINPATH_NAME",
"PYTHONPATH":["{PYTHONPATH}","/path/to/ayon-usd-resolver/dist/ayonUsdResolver/lib/python"]
}
```

## Resolver Behavior:

When a USD file is opened:
- Resolver Context is Created.
- Resolver Context creates ResolverContextCache.
    - ResolverContextCache will be linked to Resolver Context via a shared pointer. This allows USD to have one Cache on multiple resolvers. Also, if you want to explicitly construct a resolver from another resolver, you can carry the cache with you.
- USD AssetIdentifier is found.
- Resolver `_Resolve()` gets called with the data between the `@` symbols.
- `_Resolve()` checks if the path is an AYON URI path.
    - Yes: Then we get the current context (because in this resolver the resolver context interacts with the AyonCppApi and not the Resolver).
        - We ask the ResolverContext to return the path to us and the ResolverContext calls the `getAsset()` function in the cache.
        - The ResolverContextCache will then first check the PreCache and then the Responsible cache. If the ResolverContextCache found the asset, it will be returned as a strukt. If the ResolverContextCache did not find an asset, it will call the AyonCppApi and request the asset information from the server.
    - No: Then we check if the AssetIdentifier was already registered in the CommonCache and if so, we will return the cached entry. If not, we will simply resolve the path against the operating system file system the same way that USD Default Resolver Does it.

## Asset Identifier / Behavior:

The AssetIdentifier or AssetPath is always used by the resolver to convert an AYON path to a path on disk. The resolver needs some information in the path to figure out what asset you want.
1. `ayon:` is used in the `_resolve()` function in order to know if your asset is an AYON asset or not (we do this via a string view comparison).
2. `//{ProjectName}/{pathToYourFile}?product={FileName}` This is a classic AYON path that defines what asset you want, e.g., name, ayonPath, etc.
3. `version=*` version has multiple options:
    - `*`: Will tell the resolver to use the latest version no matter what.
    - `hero`: Will tell the resolver to find the pinned hero version (you should know that you have the option to set up your AYON server without hero versions, in this case, the resolver will not be able to find your asset).
    - `v001` / `vxxx`: Will allow you to pin a specific version of the asset.
4. `representation=usd` this part off the path is very important it sets the file "extention" that the resolver will search for. you can use everything that you can uploade to the server.

All together you will get an asset path like this. This asset path can be used inside of an USD and will be resolved by the asset Resolver.

`ayon://{ProjectName}/{pathToYourFile}?product={FileName}&version=*&representation=usd`


## Good To Know


1.
There is a DoxyGen file in the root of this repository. 
If you have Doxygen installed, you can simply generate the code documentation on your local machine. 
It might give a better overview of the functions and what they are supposed to do.
2.
there is currently no option to disable or control the log output of the AyonCppApi (this will be implemented soon)
- The AyonCppApi will create a `LogFile.json` in the work root directory. Usually where the work file off houdini is located (if you set the work dir manually then its not there)
    - in the near future there will be an option for disabling logging to the `logFile.json` and for setting its location. 
