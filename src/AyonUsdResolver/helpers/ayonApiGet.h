#ifndef AYON_GET_H
#define AYON_GET_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "AyonCppApi.h"
#include "appDataFoulder.h"

std::unique_ptr<AyonApi> getAyonApiFromEnv();

#endif
