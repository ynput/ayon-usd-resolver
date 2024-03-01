#ifndef CONFIG_H
#define CONFIG_H

#include <cstddef>
#include <string_view>
namespace Config {
std::string_view ayonUriIdent("ayon:");
std::string_view ayonEntetyUriIdent("ayon+entity:");
size_t ayonUriIdentSize(ayonUriIdent.size());
size_t ayonEntetyUriIdentSize(ayonEntetyUriIdent.size());
}   // namespace Config

#endif   // CONFIG_H
