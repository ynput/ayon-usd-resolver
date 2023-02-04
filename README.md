# ayon-usd-resolver
Home of AYON USD resolver plugin which resolves asset paths for [USD](https://github.com/PixarAnimationStudios/USD) and schema yon://.

The plugin works as intermediary between AYON and USD. 

High level view:

1. Some client application (call it **APP**) needs to resolve **ayon://some\_path**
1. **APP entrusts USD** (by C++ calling, by python calling, by usdresolve script calling, etc.) 
1. **USD** asset resolving mechanism **entrusts** **this plugin** to resolve ayon://some\_path.
1. This plugin sends query with **unresolved path** to **REST** (AYON REST API) where is written logic and returns **resolved path** to the plugin.
1. The plugin has **resolved path**, so returns it **to USD**
1. USD returns **resolved path to APP**.

This plugin communicates with REST API through [Boost.Beast](https://github.com/boostorg/beast). The [Boost](https://www.boost.org/) should be distributed with [USD](https://github.com/PixarAnimationStudios/USD).

Logic of this plugin is inspired by commit <https://github.com/PixarAnimationStudios/USD/commit/f482550ecc87d5b7df7dc672af6206ac4e97e55a> in official [PixarAnimationStudios](https://github.com/PixarAnimationStudios)/[**USD](https://github.com/PixarAnimationStudios/USD)** repository. The commit adds scarce commodity on the internet, working asset reolver for USD.
## **CMake**
For multiplaform development setup is used [CMake](https://cmake.org/).

Tested version of CMake was 3.25.1 on Windows and 3.25.2 on Ubuntu 22.04.1 LTS

There are used 2 CMakeLists.txt, the first one in root of source directory and the second one in subfolder src.
#### **CMakeLists.txt (in root of source directory)**
The first CMakeLists.txt in root folder of source directory is used for:

- Connection of CMake and USD through **environment variable USD**. This environment variable must store **location to root directory** where is **installed USD**.
- Finding **Boost** variables and **Python** variables
- Logging basic system setup (OS type) for easier debugging.
- Building system setup. Now it adds CMAKE\_CXX\_FLAGS for compiler on MSVC (Visual Studio).
  - **/Zc:inline-**: says compiler to not exclude unreferenced code and data
  - **/DHAVE\_SNPRINTF**:- says that really exists sprintf function
- Setup C++ version and CMake project name, minimal version of CMake, etc.
#### **CMakeLists.txt (in subfolder src)**
In this file are instructions for CMake to:

- Create shared library (**.dll** in Windows, **.so** in Unix/Linux)
- Settings of **linking paths** where **to search libraries** to link in.
- **Linking libraries** usd\_ar, usd\_arch, usd\_sdf, usd\_tf, usd\_js
- **Linking Python library**, wrapped in condition **for MSVC only**.
- **Including external directories** (for Python, USD and Boost) to include for compiler.
- **Including internal source files** of plugin.
- **Passing exported name** of library **to plugInfo.json**.
- **Installation instructions** for library and plugInfo.json.
## **Source Code**
#### **AYON\_AssetResolver**
Class stored in <SOURCE\_ROOT>/src/**AYON\_AssetResolver**(**.h**/**.cpp**)

This class **implements** methods from USD class PXR\_NS::ArResolver.
#### **AYON\_AssetResolveResult**
Class stored in <SOURCE\_ROOT>/src/**AYON\_AssetResolveResult**(**.h**/**.cpp**)

This class holds information about **resolved path**, **error message** and **boolean** information if **path was resolved**.
#### **AYON\_AssetResolvingProvider**
Interface class stored in <SOURCE\_ROOT>/src/**AYON\_AssetResolvingProvider.h**

This class has only one pure virtual method virtual YON\_AssetResolveResult Resolve(const std::string& AssetPath). This method is used as glue for future implementations of resolvers (another REST provider based on differrent logic than Boost.Beast, another type of provider, eg. direct access to database, etc.)
#### **AYON\_AssetResolvingProvider\_REST**
<SOURCE\_ROOT>/src/**AYON\_AssetResolveResult**(**.h**/**.cpp**)

In this class is written logic for communication with REST endpoint.

#### **AYON\_Logging**
<SOURCE\_ROOT>/src/Logging/**YON\_Logging**(**.h**/**.cpp**)

In these files are declared/defined AYON\_LogUtils class and macros for logging.

**Logging** is flushed to **std::cerr stream**, so do not be startled by console output, where is flushed result output for calling and logs in one pile.

To log exception call AYON\_LOG\_EXCEPTION, to log information call YON\_LOG\_INFO

## **plugInfo.json**
Plugin handles **schema yon://**. Variable@PLUG\_INFO\_LIBRARY\_PATH@ is replaced by output library name, value is created in CMakeLists.txt (in subfolder src). 

@PLUG\_INFO\_LIBRARY\_PATH@: In Windows, library suffix is .dll, in Unix .so. For library in Unix is also added prefix lib. 
## **Tested environments**
- Windows 10, Visual Studio Community 2019, USD 0.22.11, CMake 3.25.1
- Ubuntu 22.04.1 LTS, USD 0.23.02, CMake 3.25.2
# **TODO:**
1. json for configuring REST (port, host)
1. Implement contexts and anchors (requirements: mini research + testing file)

