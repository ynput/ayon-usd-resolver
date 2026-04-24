#include "resolverTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

AyonUsdResolverTokensType::AyonUsdResolverTokensType() :
mappingPairs("mappingPairs", TfToken::Immortal)
{
}

const AyonUsdResolverTokensType&
GetAyonUsdResolverTokens() {
    // Houdini tears down plugin DLL state late in process shutdown.
    // Keep the token table alive for the lifetime of the process and
    // avoid TfStaticData destructor ordering during exit.
    static const auto* tokens = new AyonUsdResolverTokensType();
    return *tokens;
}

PXR_NAMESPACE_CLOSE_SCOPE
