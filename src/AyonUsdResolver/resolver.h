#ifndef AR_AYONUSDRESOLVER_RESOLVER_H
#define AR_AYONUSDRESOLVER_RESOLVER_H

#include "pluginData/api.h"

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "redirectionFileHanlder/redirectionHanlder.h"
#include "resolverContext.h"
#include "helpers/resolutionFunctions.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class AR_AYONUSDRESOLVER_API AyonUsdResolver final: public ArResolver {
    public:
        
        AyonUsdResolver();
        virtual ~AyonUsdResolver();
        std::string _CreateIdentifier(const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const final;
        std::string _CreateIdentifierForNewAsset(const std::string &assetPath,
                                                 const ArResolvedPath &anchorAssetPath) const final;
        ArResolvedPath _Resolve(const std::string &assetPath) const final;
        ArResolvedPath _ResolveForNewAsset(const std::string &assetPath) const final;
        ArResolverContext _CreateDefaultContext() const final;
        ArResolverContext _CreateDefaultContextForAsset(const std::string &assetPath) const final;
        bool _IsContextDependentPath(const std::string &assetPath) const final;
        void _RefreshContext(const ArResolverContext &context) final;

        ArTimestamp _GetModificationTimestamp(const std::string &assetPath,
                                              const ArResolvedPath &resolvedPath) const final;
        std::shared_ptr<ArAsset> _OpenAsset(const ArResolvedPath &resolvedPath) const final;
        std::shared_ptr<ArWritableAsset> _OpenAssetForWrite(const ArResolvedPath &resolvedPath,
                                                            WriteMode writeMode) const final;
        const AyonUsdResolverContext* GetConnectedContext() const;

        const redirectionFile* getRedirectionFile() const;

        void setRedirectionFileForCtx(const std::string &rdfFilePath);

    private:
        const AyonUsdResolverContext* _GetCurrentContextPtr() const;
        AyonUsdResolverContext _fallbackContext;
        // const std::string emptyString{""};
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif   // AR_AYONUSDRESOLVER_RESOLVER_H
