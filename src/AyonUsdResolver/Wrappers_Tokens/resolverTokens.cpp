#include "resolverTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

AyonUsdResolverTokensType::AyonUsdResolverTokensType() :
mappingPairs("mappingPairs", TfToken::Immortal)
{
}

const AyonUsdResolverTokensType&
GetAyonUsdResolverTokens() {
    // Keep the token table alive for the lifetime of the process.
    static const auto* tokens = new AyonUsdResolverTokensType();
    return *tokens;
}

PXR_NAMESPACE_CLOSE_SCOPE
