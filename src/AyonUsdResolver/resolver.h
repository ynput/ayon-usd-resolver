#ifndef AR_AYONUSDRESOLVER_RESOLVER_H
#define AR_AYONUSDRESOLVER_RESOLVER_H

#include "api.h"

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "resolverContext.h"
#include "reseutionFunctions.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class AyonUsdResolver final: public ArResolver {
    public:
        AR_AYONUSDRESOLVER_API
        AyonUsdResolver();
        AR_AYONUSDRESOLVER_API
        virtual ~AyonUsdResolver();
        AR_AYONUSDRESOLVER_API
        std::string _CreateIdentifier(const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const final;
        AR_AYONUSDRESOLVER_API
        std::string _CreateIdentifierForNewAsset(const std::string &assetPath,
                                                 const ArResolvedPath &anchorAssetPath) const final;
        AR_AYONUSDRESOLVER_API
        ArResolvedPath _Resolve(const std::string &assetPath) const final;
        AR_AYONUSDRESOLVER_API
        ArResolvedPath _ResolveForNewAsset(const std::string &assetPath) const final;
        AR_AYONUSDRESOLVER_API
        ArResolverContext _CreateDefaultContext() const final;
        AR_AYONUSDRESOLVER_API
        ArResolverContext _CreateDefaultContextForAsset(const std::string &assetPath) const final;
        AR_AYONUSDRESOLVER_API
        bool _IsContextDependentPath(const std::string &assetPath) const final;
        AR_AYONUSDRESOLVER_API
        void _RefreshContext(const ArResolverContext &context) final;

        AR_AYONUSDRESOLVER_API
        ArTimestamp _GetModificationTimestamp(const std::string &assetPath,
                                              const ArResolvedPath &resolvedPath) const final;
        AR_AYONUSDRESOLVER_API
        std::shared_ptr<ArAsset> _OpenAsset(const ArResolvedPath &resolvedPath) const final;
        AR_AYONUSDRESOLVER_API
        std::shared_ptr<ArWritableAsset> _OpenAssetForWrite(const ArResolvedPath &resolvedPath,
                                                            WriteMode writeMode) const final;

    private:
        const AyonUsdResolverContext* _GetCurrentContextPtr() const;
        AyonUsdResolverContext _fallbackContext;
        const std::string emptyString{""};
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif   // AR_AYONUSDRESOLVER_RESOLVER_H
