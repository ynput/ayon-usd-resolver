#ifndef AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H
#define AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H

#include "api.h"
#include "debugCodes.h"

#include "pxr/pxr.h"
#include "pxr/usd/ar/defineResolverContext.h"
#include "pxr/usd/ar/resolverContext.h"

#include "resolverContextCache.h"

#include <map>
#include <memory>
#include <regex>
#include <string>

class AyonUsdResolverContext {
    public:
        // Constructors
        AR_AYONUSDRESOLVER_API
        AyonUsdResolverContext();
        AR_AYONUSDRESOLVER_API
        AyonUsdResolverContext(const AyonUsdResolverContext &ctx);

        // Standard Ops
        AR_AYONUSDRESOLVER_API
        bool operator<(const AyonUsdResolverContext &ctx) const;
        AR_AYONUSDRESOLVER_API
        bool operator==(const AyonUsdResolverContext &ctx) const;
        AR_AYONUSDRESOLVER_API
        bool operator!=(const AyonUsdResolverContext &ctx) const;
        AR_AYONUSDRESOLVER_API
        friend size_t hash_value(const AyonUsdResolverContext &ctx);

        // Methods
        AR_AYONUSDRESOLVER_API
        void Initialize();
        AR_AYONUSDRESOLVER_API
        void ClearAndReinitialize();

        AR_AYONUSDRESOLVER_API
        pxr::ArResolvedPath cacheFind(const std::string &key) const;

    private:
        std::shared_ptr<resolverContextCache> cache = std::make_shared<resolverContextCache>();
};

PXR_NAMESPACE_OPEN_SCOPE
AR_DECLARE_RESOLVER_CONTEXT(AyonUsdResolverContext);
PXR_NAMESPACE_CLOSE_SCOPE

#endif   // AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H
