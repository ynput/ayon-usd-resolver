# Getting Started

> **INFO** Currently, we only support specific set of DCCs and their versions, and
>   AyonUsd for building revolvers (other software packages and stand-alone
>   setups will follow).

Welcome to the Developer Docs. This document will be split into 2 portions.

1: the **Resolver Developer** part. Lets take a look at what tools and library's
we use and how we use them.

2: documentation for **AYON** developers that want to use the resolver but don't
want to work on the source code. it will outline all the things the resolver
offers and how you can control them.

# Resolver Developers 

### Dependencies

- [AyonCppApi](https://github.com/ynput/ayon-usd)\n
  Internal AyonCppApi. Used for communication with the server and specifically
  designed to allow for Fast resolution.
- [HttpLib](https://github.com/yhirose/cpp-httplib)\n
  Header only Http lib that we use in the Cpp api to communicate with the
  server.
- [USD](https://github.com/PixarAnimationStudios/OpenUSD)\n
  OpenUsd library from Pixar



### compilation

The `build.py` script located in the `scripts` folder serves as our primary build script, superseding the previous `.sh` and `.bat` files. We plan to replace this with 'Build Stage' using Ayon Automator in the future.

Target tools for compilation are organized into separate `BuildPlugins`. To compile for a specific tool, select the corresponding `BuildPlugin` and provide the required environment variables for that tool. Subsequently, execute `build.py`, ensuring you specify the chosen `BuildPlugin` as an environment variable.

#### Varlibes:
`COMPILEPLUGIN` = Set this variable to the relative path of the desired build plugin, found within the `BuildPlugins` folder in the repository root. For example, if using `HouLinux/LinuxPy310Houdini20`.

## Developer Guidelines 

### Testing 

The USD Resolver currently does not employ Red-Green development. However, post-alpha release, we plan to integrate Ayon Automator.

**Issue Tracking**
We maintain all our work in GitHub (GH) issues and create branches from these issues. Here are the three types of issue options:

- **Bug Report**: For reporting any bugs or glitches.
- **Feature Request**: For suggesting new features or improvements.
- **Proposal**: For significant changes or complex feature implementations, allowing discussion on technical approaches and review of past decisions.

**Choose your Issue Type**
Depending on the scale of work, you can pick the issue type that best fits. However, for substantial features with high complexity, it's crucial to create a Proposal. This ensures thoughtful debate around technical implementation and allows us to revisit older proposals for context.


## Working with Build Plugins

### Naming Build Plugins

Build Plugins Naming schema is as follows:
`{AppName}{PlatfromName}/{AppName}{AppVersion}_Py{pythonVersion}_{PlatfromName}.cmake`
PlatformName options = {Win, Linux, Mac, specific Os Name}




## Resolver Internals


### Resolver Behavior:

On USD Init:

- AyonUsdResolver library will be loaded, and a resolverContextCache and a
  globally accessible
  [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) will be
  created

When a USD file is opened:

- Resolver Context is created.

  - every resolver context has a
    [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) that will
    point to the global resolverContextCache

When a USD
[AssetIdentifier](https://openusd.org/release/glossary.html#usdglossary-assetinfo)
is found.

- `_Resolve()` gets called with the data between the ['@'](https://openusd.org/release/glossary.html#usdglossary-asset) symbols.
- `_Resolve()` checks if the path is an AYON URI path.
  - **Yes?** Then we get the current context (because in this resolver, the
    resolver context interacts with the
    [AyonCppApi](https://github.com/ynput/ayon-cpp-api/) and not the Resolver).
    - We ask the ResolverContext to return the path to us, and the
      ResolverContext calls the `getAsset()` function in the
      resolverContextCache.
    - The resolverContextCache will then first check the PreCache and then the
      Responsible cache. If the resolverContextCache finds the asset, it will return a assetIdent. If the resolverContextCache does not find an
      asset, it will call the **AyonCppApi** and request the asset information
      from the server.
  - **No?** Then we check if the AssetIdentifier was already registered in the
    CommonCache, and if so, we will return the cached entry. If not, we will
    resolve the path against the operating file system the same way that USD
    Default Resolver does it.



### Asset Identifier Behavior:

The AssetIdentifier or AssetPath converts an AYON path to a disk path for the resolver:

1. `ayon:` signifies an AYON asset (detected via string view comparison).
2. `//{ProjectName}/{path/to/ayon/folder}?product={FileName}` defines the desired Ayon folder.
3. Version options:
   - `latest`: Uses latest version available.
   - `hero`: Searches for pinned hero version, if configured.
   - Specific version number (e.g., `v001`): Uses specified product version.

The complete asset path format is: `ayon://{ProjectName}/{path/to/ayon/folder}?product={FileName}&version=latest&representation=usd`

> CPP files may contain `TF_DEBUG().Msg();` and one of the enum values (`AYONUSDRESOLVER_RESOLVER`, `AYONUSDRESOLVER_RESOLVER_CONTEXT`). These control debug message output. Keep these in your environment variables for flexibility.




# Ayon Developers

## Pythonic Usage via USD

First of all, if you setup the resolver via the AyonUsd addon usd will
automatically use it as the default resolver meaning every time you call a
resolver AyonUsdResolver will be used.

But you have more fine grained control if you want to:

```py
from pxr import Ar
from usdAssetResolver import AyonUsdResolver
Ar.SetPreferredResolver("AyonUsdResolver")
```


> **note**
> When you get a resolver via `Usd.Ar` API you will need to get an explicit context to 
> edit the global context as `Ar.GetResolver` will return a higher order class and 
> not `AyonUsdResolver` class.

```py
resolver = Ar.GetResolver()
context = AyonUsdResolver.ResolverContext()
```


If you want fine control over the resolver you will be able to get the connected
context via `GetConnectedContext`\n

tip Dropping cache entries
Using the `resolver.GetConnectedContext()` is also the recommended way to access the `dropCache` function for the
resolver.


```py
explicit_resolver = AyonUsdResolver.Resolver()
explicit_resolver_context = explicit_resolver.GetConnectedContext()
```

To access the Python bindings for the AyonUsdResolver:
```py
from usdAssetResolver import AyonUsdResolver
```

`Ar.SetPreferredResolver("AyonUsdResolver")` this command allows you to set a
preferred Resolver. If you use the AyonUsd addon you wont need to set this line
as we register the resolver as the main resolver. But if you have a different
resolver set as Main you might need to use this command.

`resolver = Ar.GetResolver()` there are multiple ways to get access to a
resolver instance but using the "Default" Usd option is usually the best idea as
it will return the resolver for the current context.

`context = AyonUsdResolver.ResolverContext()` accessing an ResolverContext
instance is useful if you want to manipulate the Global resolver cache.

## Pinning

the AyonUsdResolver has a feature called Pinning Support.\n
In short _pinning support_ allows you to load a pinning file and disconnect the
AyonCppApi.\n
In the implementation this boils down to a static Memory cache that bypasses the
resolver Logic.\n

> **note** Why use pinning?
> When running a resolver on a farm with many workers (render nodes) 
> you might not want them all to call the Ayon server to avoid impacting 
> the server performance. This is because USD Resolvers will resolve paths
> one by one, potentially resulting in many calls.

**How to use it ?**\n
There are 3 Env variables that control the Pinning file support

`ENABLE_STATIC_GLOBAL_CACHE`\n
Enables or Disables static Cache creation. This effectively allows you to enable
or disable Pinning file support.\n
as you might want to have a pinning file in your env vars but for e.g debugging
you don't want to activate the feature.

`PINNING_FILE_PATH`\n
This is a path to the pinning file.

`PROJECT_ROOTS`\n
When running the resolver against the AYON server the CppApi will query the
`Get Project Site Roots` endpoint and get a `Dict[str,str]` of
{root[RootOverwriteKey]}=Val overwrites.\n
when running with a pinning file you will need to set this Dict[str,str] as an
ENV variable. e.g:`{Key}:{Path},{Key}:{Path}` it's not a problem to have
duplicates in the keys but the Cache stores the data as an Unordered_Map so it
will end up deduplicating the Keys. But you can't have spaces in the list as
they are not removed and will be interpreted as part of the Key or Value.


**example**\n
setup the resolver for pinning support. we empty all the AYON c++ api keys just
for example you can simply not set them.

```bash
export AYON_API_KEY=""
export AYON_SITE_ID=""
export AYON_SERVER_URL=""
export AYON_PROJECT_NAME=""

export AYONLOGGERLOGLVL=""
export AYONLOGGERFILELOGGING=""
export AYONLOGGERFILEPOS=""

export ENABLE_STATIC_GLOBAL_CACHE="true"
export PINNING_FILE_PATH="/path/to/pinning.json"
export PROJECT_ROOTS="key:/val,key:/val"

export TF_DEBUG=""
```

```py
stage = Usd.Stage.Open(pinning_file_json["ayon_pinning_data_entry_sceene"])
print(stage.ExportToString())
```

PS: its interesting to know that when you generate a pinning file via the
AYON-USD addon the json file will have a key named

`ayon_pinning_data_entry_scene`.\n
This should always be the path used to open the stage Otherwise the pinning file
might not have the correct AssetIdentifier stored.\n
aka: if this key points to a local path and you use an URI that points to the
local path the resolver wont be able to resolve.

## Controlling the cache

`context = AyonUsdResolver.ResolverContext()` there are multiple ways to control
the cache of a resolver. but if your resolver uses the global cache you can
simply create a new ResolverContext and access the cache control functions to

affect the global cache.\n
This does not work if you disconnected the Global cache from your resolver.

`context.deleteFromCache(AssetIdentifier)` delete an individual cached entry.
`context.clearCache()` clear the connected cache. 


> **note**
> It is important to understand that by default a Resolver will be connected to the global cache
> and this call will delete all entries in the global cache not just the ones that
> where added by this resolver.

`context.dropCache()` allows you do disconnect from the current cache object and
initialize a new one. This can be useful if you want to disconnect from the
global cache and have a Resolver local cache instead.

`explicit_resolver = AyonUsdResolver.Resolver()` if you need to create an
explicit resolver because you want to e.g pass a specific instance into a stage
or you want to disconnect from the Global Cache you should access the Context

connected to this specific resolver.\n
`explicit_resolver_context = explicit_resolver.GetConnectedContext()` all the
other functions stay the same.

**Examples**

Deleting cached entry's

```py
resolver = Ar.GetResolver()
context = AyonUsdResolver.ResolverContext()

# Delete From the Global Cache
asset_identifier: str="ayon://{ProjectName}/{path/to/ayon/folder}?product={FileName}&version=latest&representation=usd"
context.deleteFromCache(asset_identifier)

# Clear the Global Cache
context.clearCache()
```

Disconnecting from the global cache

```py
explicit_resolver = AyonUsdResolver.Resolver()
explicit_resolver_context = explicit_resolver.GetConnectedContext()

# Drooping the cache so that the resolver will generate its own dedicated cache instance
explicit_resolver_context.dropCache()

# Delete from cache and clearCache will also work with an explicit_resolver_context
```
