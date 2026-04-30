#ifndef AR_AYONUSDRESOLVER_TOKENS_H
#define AR_AYONUSDRESOLVER_TOKENS_H

#include "../pluginData/api.h"

#include <pxr/pxr.h>
#include <pxr/base/tf/token.h>

PXR_NAMESPACE_OPEN_SCOPE

struct AyonUsdResolverTokensType {
        AR_AYONUSDRESOLVER_API AyonUsdResolverTokensType();

        const TfToken mappingPairs;
};

AR_AYONUSDRESOLVER_API
const AyonUsdResolverTokensType& GetAyonUsdResolverTokens();

PXR_NAMESPACE_CLOSE_SCOPE

#endif   // AR_AYONUSDRESOLVER_TOKENS_H
