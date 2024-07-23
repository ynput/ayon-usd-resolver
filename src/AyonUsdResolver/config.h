#ifndef CONFIG_H
#define CONFIG_H

#include <array>
#include <cstdint>
#include <string_view>

#define PRECACHE_SIZE                       64

#define ENABLE_STATICK_GLOBAL_CACHE_ENV_KEY "ENABLE_STATICK_GLOBAL_CACHE"

#define PROJECT_ROOTS_ENV_KEY               "PROJECT_ROOTS"
#define PINNING_FILE_PATH_ENV_KEY           "PINNING_FILE_PATH"

namespace Config {

// TODO implement Config as lock less read only Singleton as there is no need to write to the config nor is there a need
// to have this data initiated twice
struct AyonUriConfigStruct {
        std::array<std::string_view, 2> ayonUriOptions = {"ayon:", "ayon+entity:"};
        std::array<uint8_t, 2> ayonUriOptionsSize = {uint8_t(5), uint8_t(12)};
};

}   // namespace Config

#endif   // CONFIG_H
