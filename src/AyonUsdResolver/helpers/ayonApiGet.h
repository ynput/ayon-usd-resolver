#ifndef AYON_GET_H
#define AYON_GET_H

#include <memory>

#include "AyonCppApi.h"

std::unique_ptr<AyonApi> getAyonApiFromEnv();

#endif
