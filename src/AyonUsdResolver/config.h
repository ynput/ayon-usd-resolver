#ifndef CONFIG_H
#define CONFIG_H

#include <cstddef>
#include <string_view>
namespace Config {
std::string_view ayonUriIdent("ayon:");
size_t ayonUriIdentSize(ayonUriIdent.size());
}   // namespace Config

#endif   // CONFIG_H
