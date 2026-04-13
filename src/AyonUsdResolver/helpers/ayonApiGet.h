#ifndef AYON_GET_H
#define AYON_GET_H

#include "AyonCppApi.h"

#include <memory>

std::unique_ptr<AyonApi> getAyonApiFromEnv();

#endif
