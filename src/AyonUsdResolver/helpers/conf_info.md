**USD Resolver and C++ API Environment Variables**

The USD resolver and C++ API have a list of environment variables that can be used to configure its behavior. Here's a rundown:

- **`AYON_USD_RESOLVER_LOG_LVL`**: Uses `spdlog` inside the C++ API. This key configures the logging level.
- **`AYON_USD_RESOLVER_LOG_FILE_ENABLED`**: Allows file logging, which can be disabled if not needed.
- **`AYON_USD_RESOLVER_LOG_FILE`**: Provides a path for file logging; useful when set manually.
- **`AYON_USD_RESOLVER_LOG_KEYS`**: Enables special logging keys inside the C++ API for deeper insights. Useful during development but may spam logs otherwise.
- **`AYON_USD_RESOLVER_ENABLE_PINNING`**: Enables/disables pinning, a feature supporting convenience toggling between test runs without unsetting the pinning file.
- **`AYON_USD_RESOLVER_PINNING_FILE`**: The pinning file's position. It can be relative or absolute; `std::canonical` will make it absolute based on where the resolver gets initialized.
- **`AYON_USD_RESOLVER_PINNING_ROOTS`**: When using a pinning file, provide local work roots to cut server communication.

These variables are provided by Ayon when running in an Ayon environment for server communication:

- `AYON_API_KEY`
- `AYON_SERVER_URL`
- `AYON_SITE_ID`
- `AYON_PROJECT_NAME`

Additionally, there's a USD variable used to inform TF debug:

- **`TF_DEBUG`**

**Renamed Environment Variables (for reference):**

- `AYONLOGGERLOGLVL` → `AYON_USD_RESOLVER_LOG_LVL`
- `AYONLOGGERFILELOGGING` → `AYON_USD_RESOLVER_LOG_FILE_ENABLED`
- `AYONLOGGERFILEPOS` → `AYON_USD_RESOLVER_LOG_FILE`
- `AYON_LOGGIN_LOGGIN_KEYS` → `AYON_USD_RESOLVER_LOG_KEYS`

- `ENABLE_STATIC_GLOBAL_CACHE` → `AYON_USD_RESOLVER_ENABLE_PINNING`
- `PINNING_FILE_PATH` → `AYON_USD_RESOLVER_PINNING_FILE`
- `PROJECT_ROOTS` → `AYON_USD_RESOLVER_PINNING_ROOTS`

**Note:** Some variables cannot be renamed:

- `AYON_API_KEY`
- `AYON_SERVER_URL`
- `AYON_SITE_ID`
- `AYON_PROJECT_NAME`
- `TF_DEBUG`D
