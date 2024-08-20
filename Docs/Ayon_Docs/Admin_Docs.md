# Admin Docs

### General

A few things to begin with.

1. The AyonUsd addon will setup the resolver via Ayon Server settings and in
   most cases this will be enough for Studio Deployment.
2. You can setup the resolver via environment variables like any other OpenUsd addon
   just in case you need some special Requirements.

### How to setup a Resolver

**The Env Variables**\
USD_ASSET_RESOLVER:

- This variable tells Usd what resolver to use and where to find it (this will
  not overwrite the default resolver as a fallback).

TF_DEBUG:

- This variable allows you to choose what Debug messages will be printed.
  - In the CPP files, you find TF_DEBUG().Msg();\
    they always have one of two Enum Values AYONUSDRESOLVER_RESOLVER or
    AYONUSDRESOLVER_RESOLVER_CONTEXT these allow you to select what debug
    messages will be printed.
  - If you want the resolver to be silent, then you can leave this value empty.

LD_LIBRARY_PATH:

- it describes where the C++ dynamic library files can be found for the
  resolver.

PXR_PLUGINPATH_NAME:

- This is also a variable for Usd, and it might look like you're supposed to
  place the AyonUsdResolver name in here, but you're actually putting the path
  to the PluginInfo.json folder into this variable.

PYTHONPATH:

- Exposes the Python wrapper functions from the resolver.

AYONLOGGERLOGLVL:

- This Environment variable allows you to set the log level for the CppApi.
  - INFO,ERROR,WARN,CRITICAL,OFF

AYONLOGGERFILELOGGING:

- allows you to enable or disable file logging in CppApi.
  - OFF,ON

AYONLOGGERFILEPOS:

- allows you to set a file path for the CppApi logging.
  - /path/to or relPath

Here an example that would work with the Environment setup inside Ayon server.
This should be setup per Software Package

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
