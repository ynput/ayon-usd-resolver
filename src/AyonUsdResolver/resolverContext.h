#ifndef AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H
#define AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H

#include "api.h"
#include "debugCodes.h"

#include "pxr/pxr.h"
#include "pxr/usd/ar/defineResolverContext.h"
#include "pxr/usd/ar/resolverContext.h"

#include <map>
#include <memory>
#include <regex>
#include <string>

/* Data Model
We use an internal data struct that is accessed via a shared pointer
as Usd currently creates resolver context copies when exposed via python
instead of passing thru the pointer. This way we can send
> ArNotice::ResolverChanged(*ctx).Send();
notifications to the stages.
> See for more info:
https://groups.google.com/g/usd-interest/c/9JrXGGbzBnQ/m/_f3oaqBdAwAJ
*/
struct AyonUsdResolverContextInternalData {
        std::string mappingFilePath;
        std::map<std::string, std::string> cachingPairs;
};

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

    private:
        std::shared_ptr<AyonUsdResolverContextInternalData> data
            = std::make_shared<AyonUsdResolverContextInternalData>();
};

PXR_NAMESPACE_OPEN_SCOPE
AR_DECLARE_RESOLVER_CONTEXT(AyonUsdResolverContext);
PXR_NAMESPACE_CLOSE_SCOPE

#endif   // AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H
