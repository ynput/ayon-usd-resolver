#ifndef CONFIG_H
#define CONFIG_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace Config {

struct AyonUriConfigStruct {
        std::array<std::string_view, 2> ayonUriOptions = {"ayon:", "ayon+entity:"};
        std::array<uint8_t, 2> ayonUriOptionsSize
            = {uint8_t(5), uint8_t(12)};
};

}   // namespace Config

#endif   // CONFIG_H
