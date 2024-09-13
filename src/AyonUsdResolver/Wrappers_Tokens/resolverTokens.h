#ifndef AR_AYONUSDRESOLVER_TOKENS_H
#define AR_AYONUSDRESOLVER_TOKENS_H

#include "../pluginData/api.h"

#include <pxr/pxr.h>
#include <pxr/base/tf/staticTokens.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct AyonUsdResolverTokensType {
        AR_AYONUSDRESOLVER_API AyonUsdResolverTokensType();

        const std::vector<TfToken> allTokens;
};

extern AR_AYONUSDRESOLVER_API TfStaticData<AyonUsdResolverTokensType> AyonUsdResolverTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif   // AR_AYONUSDRESOLVER_TOKENS_H
