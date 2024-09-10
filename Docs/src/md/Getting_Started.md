# Getting Started

> **INFO** Currently, we only support specific set of DCCs and their versions, and
   AyonUsd for building revolvers (other software packages and stand-alone
   setups will follow).

## Usage

### compilation

The `build.py` script located in the `scripts` folder serves as our primary build script, superseding the previous `.sh` and `.bat` files. We plan to replace this with 'Build Stage' using Ayon Automator in the future.

Target tools for compilation are organized into separate `BuildPlugins`. To compile for a specific tool, select the corresponding `BuildPlugin` and provide the required environment variables for that tool. Subsequently, execute `build.py`, ensuring you specify the chosen `BuildPlugin` as an environment variable.

#### Varlibes:


`COMPILEPLUGIN` = Set this variable to the relative path of the desired build plugin, found within the `BuildPlugins` folder in the repository root. For example, if using `HouLinux/LinuxPy310Houdini20`.

## Developing for this tool set

### General

**USD Resolver**

The USD Resolver currently does not employ Red-Green development. However, post-alpha release, we plan to integrate Ayon Automator.

**Issue Tracking**
We maintain all our work in GitHub (GH) issues and create branches from these issues. Here are the three types of issue options:

- **Bug Report**: For reporting any bugs or glitches.
- **Feature Request**: For suggesting new features or improvements.
- **Proposal**: For significant changes or complex feature implementations, allowing discussion on technical approaches and review of past decisions.

**Choose your Issue Type**
Depending on the scale of work, you can pick the issue type that best fits. However, for substantial features with high complexity, it's crucial to create a Proposal. This ensures thoughtful debate around technical implementation and allows us to revisit older proposals for context.

### Testing

Will be integrated with the switch to Red Green Development


## Developer Information

The BuildPlugins naming schema follows this pattern:
`{AppName}{PlatformName}/{AppName}{AppVersion}_Py{pythonVersion}_{PlatformName}.cmake`
Accepted `PlatformName` options are: {Win, Linux, Mac, specific OS Name}.

### Resolver Behavior:

Upon USD initialization:

- Loads the AyonUsdResolver library and creates a global resolverContextCache with an associated shared_ptr.

When opening a USD file:

- Creates a Resolver Context.
  - Each resolver context holds a shared_ptr referencing the global resolverContextCache.

Upon encountering a USD [AssetIdentifier](https://openusd.org/release/glossary.html#usdglossary-assetinfo):

- Calls [_Resolve()](classAyonUsdResolver.html#ac21450dc819a8238d3adeaf023b7ac69) function using data between `@` symbols.
- Checks if the path is an AYON URI path via [_IsAyonPath()](resolutionFunctions_8h.html#ad3ae0b89367898008d012cf6c1df2f26):
  - **Yes**: Retrieves current context and requests asset path from ResolverContext, which invokes [getAsset()](classresolverContextCache.html#a9791b043238afd88047c161b8ccb0c91) in resolverContextCache.
    - If found, returns a assetIdent. If not, queries the server via [AyonCppApi](https://ynput.github.io/ayon-cpp-api/).
  - **No**: Checks if AssetIdentifier is registered in CommonCache; if so, returns cached entry. Otherwise, resolves path against OS file system like USD Default Resolver.

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
