# AYON USD Resolver
The AYON USD Resolver is a [Pixar's USD](https://openusd.org) [ArResolver URI Resolver Plugin](https://openusd.org/release/api/ar_page_front.html#ar_uri_resolvers) that resolves URIs with the schema `ayon://` found in a USD file into actual filepaths.

The plugins relies on a [Ayon instance running](https://github.com/ynput/ayon-docker), which will provide the file path of the URI via the `/api/usd` endpoint, at a high level it works as portrayed below:

```
 ┌──────────────────────────────┐
 │                              │
 │ Open USD file.               │
 │                              │
 │            │                 │
 │            ▼                 │
 │                              │
 │ USD's ArResolver finds       │
 │ `ayon://` URI.               │
 │                              │
 │            │                 │
 │            ▼                 │
 │                              │
 │ AYON USD Resolver queries    │
 │ the AYON Instance `/api/usd` │
 │ with the URI and receives a  │
 │ filepath.                    │
 │                              │
 │            │                 │
 │            ▼                 │
 │                              │
 │ USD replaces URI with actual │
 │ the actual file path.        │
 │                              │
 └──────────────────────────────┘
```

## Usage Instructions
To Register the plugin you can either:
 * Set the environment variable `PXR_PLUGINPATH_NAME` to the path to `pluginInfo.json`.
 * Copy both `pluginInfo.json` and `libAYON_AssetResolverPlugin(.so|.dll)` into the `/plugin/usd` subfolder in the USD install.

## Building Instructions
In order to build this project you'll need:
 * A C/C++ compiler.
 * [Cmake](https://cmake.org/) minimum version 3.13.0, tested 3.25.1 on Windows and 3.25.2 on Ubuntu 22.04.1 LTS.
 * The USD binaries, **with an environment variable `USD` that points to these** to get USD you can:
    - Build it from source: https://github.com/PixarAnimationStudios/USD/blob/release/BUILDING.md
    - Download NVIDIA's pre-built binaries: https://developer.nvidia.com/usd#bin
 * [Boost](https://boost.org/) which is normally vendored in USD.
 * [Python](https://python.org/) with the Development headers.

From the top level of the project do:
 ```bash
 cmake build
 make -C build
 ```

If the build fails at finding the `pyconfig.h` file, you can specify the path of the Python headers with:
 ```bash
 CPLUS_INCLUDE_PATH=${CPLUS_INCLUDE_PATH}:<path-to-the-python-install>/include/pythonX.Y make -C build
 ```

Upon completion, the plugin files will be found int `build/src/`:
 * `pluginInfo.json`.
 * `libAYON_AssetResolverPlugin(.so|.dll)` (Depending on the platform you are building).

## Testing Instructions
In order to test this plugin you got to options:
 A) Spin up an AYON instance (or use an existing one) where you have published assets with disk representations
 B) Use the provided `mock_server.py` - Which you can start with `python mock_server.py`
 
In any case, you should specify the environment variable `AYON_SERVER_URL` which the plugin will read to know ther to do the REST call.

Then you can do `usdresolve ayon://foo/bar/bar_010_0010?subset=model?version=v017` this is a hardcoded URI in the `mock_server.py` if you are using a live instance, you can get an asset URI by clicking the "copy" icon at the top bar in the AYON website, when viewing an asset.

## The CMake build explained
This plugin communicates with the AYON REST API through [Boost.Beast](https://github.com/boostorg/beast) included in `Boost` and is inspired by the following commit <https://github.com/PixarAnimationStudios/USD/commit/f482550ecc87d5b7df7dc672af6206ac4e97e55a> in official [**USD**](https://github.com/PixarAnimationStudios/USD) repository.

There are two `CMakeLists.txt` files:

At the project root:
 1) Set up project name and C++ version and print debugging info.
 2) Set up CMAKE flags based on platform (`/Zc:inline-` and `/DHAVE_SNPRINTF` when using `msvc`).
 3) Find `Boost` and `Python` installs.
 4) Set `USD` related variables, this relies on a `USD` environment variable being set: `USD_GENSCHEMA`, `USD_INCLUDE_DIR` and `USD_LIBRARY_DIR` used in the next `CMakeLists/txt`.
 5) Add the `src` sub-directory.

Inside the `src` directory:
 1) Create shared library (**.dll** in Windows, **.so** in Unix/Linux).
 2) Settings of **linking paths** where **to search libraries** to link in.
 3) **Linking to the USD libraries**: `usd/ar`, `usd/arch`, `usd/sdf`, `usd/tf`, `usd/js`.
 4) **Linking Python library**, wrapped in condition **for MSVC only**.
 5) **Including external directories** (for Python, USD and Boost) to include for compiler.
 6) **Including internal source files** of plugin.
 7) **Passing exported name** of library **to plugInfo.json**.
 8) **Installation instructions** for library and plugInfo.json.


## What does the AYON USD Resolver exactly does

All the actual code is found in the `src` sub-directory:
 * `AYON_AssetResolver(.h/.cpp)` - This class **implements** methods from USD class `PXR_NS::ArResolver`.
 * `AYON_AssetResolveResult(.h/.cpp)` - This class holds information about **resolved path**, **error message** and **boolean** information if **path was resolved**.
 * `AYON_AssetResolvingProvider.h` - This class has only one pure virtual method virtual `AYON_AssetResolveResult Resolve(const std::string& AssetPath)`. This method is used as glue for future implementations of resolvers (another REST provider based on differrent logic than Boost.Beast, another type of provider, eg. direct access to database, etc.)
 * `AYON_AssetResolveResult(.h/.cpp)` - This class is written logic for communication with REST endpoint.
 * `AYON_Logging(.h/.cpp)` - In these files are declared/defined `AYON_LogUtils` class and macros for logging, **Logging** is flushed to `std::cerr stream`, so do not be startled by console output, where is flushed result output for calling and logs in one pile. To log exception call `AYON_LOG_EXCEPTION`, to log information call `AYON_LOG_INFO`.
 * `plugInfo.json` - Plugin Plugin handles schema `ayon://`. Variable `@PLUG_INFO_LIBRARY_PATH@` is replaced by output library name (`.dll` on Windows, `.so` on Unix), value is created in CMakeLists.txt (in subfolder src). 


## Tested environments
- Windows 10, Visual Studio Community 2019, USD 0.22.11, CMake 3.25.1
- Ubuntu 22.04.1 LTS, USD 0.23.02, CMake 3.25.2
- Rocky Linux 8, USD 23.05, CMake 3.26.3

## TODO
* Read the `AYON_SERVER_URL` and `AYON_API_KEY` environment variables for the REST API Authentication.
* Remove the `node_test_server` and mock the REST API.
* Implement contexts and anchors (requirements: mini research + testing file).

