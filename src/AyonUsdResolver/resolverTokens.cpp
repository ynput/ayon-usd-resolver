#include "resolverTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

AyonUsdResolverTokensType::AyonUsdResolverTokensType() :
    mappingPairs("mappingPairs", TfToken::Immortal),
    allTokens({
        mappingPairs
    })
{
}

TfStaticData<AyonUsdResolverTokensType> AyonUsdResolverTokens;

PXR_NAMESPACE_CLOSE_SCOPE
