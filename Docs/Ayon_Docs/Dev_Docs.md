# Dev Docs

### Into

Welcome to the Developer Docs. this document will be split into 2 portions.

1: documentation for **AYON** developers that want to use the resolver but dont
want to work on the source code. it will outline all the things the resolver
offers and how you can control them.\
2: the **Resolver Developer** part. this portion will talk about what we use for
the resolver how we use it and

## AyonDev Documentation

### Pythonic Usage via USD

first of all. if you setup the resolver via the AyonUsd addon usd will
automatically use it as the default resolver meaning every time you call a resolver
AyonUsdResolver will be used.

but you have a bit more fine grain controle if you want to.

```py
from pxr import Ar
from usdAssetResolver import AyonUsdResolver
Ar.SetPreferredResolver("AyonUsdResolver")
```

Get a resolver via `Usd.Ar` api you will need to get a explicit context to edit
the global context as Ar.GetResolver will return a higher order class and not
AyonUsdResolver class

```py
resolver = Ar.GetResolver()
context = AyonUsdResolver.ResolverContext()
```

If you want fine control over the resolver you will be able to get the connected
context via `GetConnectedContext`\
this is also the recommended way to get access to the dropCache function for the
resolver

```py
explicit_resolver = AyonUsdResolver.Resolver()
explicit_resolver_context = explicit_resolver.GetConnectedContext()
```

`from usdAssetResolver import AyonUsdResolver`python bindings for the
AyonUsdResolver.

`Ar.SetPreferredResolver("AyonUsdResolver")` this command allows you to set a
preferred Resolver. if you use the AyonUsd addon you wont need to set this line
as we register the resolver as the main resolver. but if you have a different
resolver set as Main you might need to use this command.

`resolver = Ar.GetResolver()` there are multiple ways to get access to an
resolver instance but using the "Default" Usd option is usually the best idea as
it will return the resolver for the current context.

`context = AyonUsdResolver.ResolverContext()` accessing an ResolverContext
instance is useful if you want to manipulate the Global resolver cache.

### Pinning

the AyonUsdResolver has a feature called Pinning Support.\
in short pinning support allows you to load a pinning File and disconnect the
AyonCppApi.\
why would you do that ? When running a resolver on a farm with many many
systems(Render Nodes) you might not want them all to call the Ayon server to
avoid impacting the server performance.

**How to use it ?**\
There are 3 Env variables that control the Pinning file support

`ENABLE_STATICK_GLOBAL_CACHE`\
Enables or Disables static Cache creation. This effectively allows you to enable
or disable Pinning file support.\
as you might want to have a pinning file in your env vars but for e.g debugging
you don't want to activate the feature.

`PINNING_FILE_PATH`\
this is a path to the pinning file.

`PROJECT_ROOTS`\
when running the resolver against the AYON server the CppApi will query the
`Get Project Site Roots` endpoint and get a Dict[str,str] of
{root[RootOverwriteKey]}=Val overwrites.\
when running with a pinning file you will need to set this Dict[str,str] as an
ENV variable. e.g:`{Key}:{Path},{Key}:{Path}"` it's not a problem to have
duplicates in the keys but the Cache stores the data as an Unordered_Map so it
will end up deduplicating the Keys. But you can't have spaces in the list as they
are not removed and will be interpreted as part of the Key or Value.

### Controlling the cache

`context = AyonUsdResolver.ResolverContext()` there are multiple ways to control
the cache of a resolver. but if your resolver uses the global cache you can
simply create a new ResolverContext and access the cache control functions to
affect the global cache.\
this does not work if you disconnected the Global cache from your resolver.

`context.deleteFromCache(AssetIdentifier)` this would allow you to delete an
individual cached entry.\
`context.clearCache()` allows you to clear the connected cache. it is important
to understand that by default a Resolver will be connected to the global cache
and this call will delete all entry's in the global cache not just the ones that
where added by this resolver.\
`context.dropCache()` allows you do disconnect from the current cache object and
initialize a new one. This can be useful if you want to disconnect from the
global cache and have a Resolver local cache instead.

`explicit_resolver = AyonUsdResolver.Resolver()` if you need to create an
explicit resolver because you want to e.g pass a specific instance into a stage
or you want to disconnect from the Global Cache you should access the Context
connected to this specific resolver.\
`explicit_resolver_context = explicit_resolver.GetConnectedContext()` all the
other functions stay the same.

## Resolver Developer Docs

**Naming BuildPluingins**

BuildPluingins Naming schema is as follows:
`{AppName}{PlatfromName}/{AppName}{AppVersion}_Py{pythonVersion}_{PlatfromName}.cmake`
PlatfromName options = {Win, Linux, Mac, specific Os Name}

### All About this Project

**Dependencys**

- [AyonCppApi](https://github.com/ynput/ayon-usd)\
  Internal AyonCppApi. Used for communication with the server and specifically
  designed to allow for Fast resolution.
- [HttpLib](https://github.com/yhirose/cpp-httplib)\
  Header only Http lib that we use in the Cpp api to communicate with the
  server.
- [USD](https://github.com/PixarAnimationStudios/OpenUSD)\
  OpenUsd library from Pixar

### Resolver Behavior:

On USD Init:

- AyonUsdResolver library will be loaded, and a `ResolverContextCache` and a
  globally accessible
  [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) will be
  created

When a USD file is opened:

- Resolver Context is created.

  - every resolver context has a
    [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) that will
    point to the global `ResolverContextCache`

When a USD
[AssetIdentifier](https://openusd.org/release/glossary.html#usdglossary-assetinfo)
is found.

- `_Resolve()` gets called with the data between the
  [@](https://openusd.org/release/glossary.html#usdglossary-asset) symbols.
- `_Resolve()` checks if the path is an AYON URI path.
  - **Yes?** Then we get the current context (because in this resolver, the
    resolver context interacts with the
    [AyonCppApi](https://github.com/ynput/ayon-cpp-api/) and not the Resolver).
    - We ask the ResolverContext to return the path to us, and the
      ResolverContext calls the `getAsset()` function in the
      `ResolverContextCache`.
    - The `ResolverContextCache` will then first check the PreCache and then the
      Responsible cache. If the `ResolverContextCache` finds the asset, it will
      be returned as a struct. If the `ResolverContextCache` does not find an
      asset, it will call the **AyonCppApi** and request the asset information
      from the server.
  - **No?** Then we check if the AssetIdentifier was already registered in the
    CommonCache, and if so, we will return the cached entry. If not, we will
    resolve the path against the operating file system the same way that USD
    Default Resolver does it.

### Asset Identifier / Behavior:

The AssetIdentifier or AssetPath is always used by the resolver to convert an
AYON path to a path on disk. The resolver needs some information in the path to
figure out what asset you want.

1. `ayon:` is used in the `_resolve()` function to know whether your asset is an
   AYON asset or not (done via a string view comparison).
2. `//{ProjectName}/{path/to/ayon/folder}?product={FileName}` This is a classic
   AYON path that defines what Ayon folder you want, e.g., sequences/sh010,
   assets/characters/bob, etc.
3. `version=latest` version has multiple options:

   - `latest`: Will tell the resolver to use the latest version no matter what.
   - `hero`: this should be avoided in most cases with usd as our internal hero
     system has not been designed for that.
   - `v001` _(or whatever you put in your template)_: Will allow you to use a
     specific version of the product.

4. `representation=usd`: This part of the path is very important; it sets the
   file "extension" that the resolver will search for. You can use everything
   that you can upload to the server.

All together, you will get an asset path like this. This asset path can be used
inside of USD and will be resolved by the asset Resolver.

`ayon://{ProjectName}/{path/to/ayon/folder}?product={FileName}&version=latest&representation=usd`
