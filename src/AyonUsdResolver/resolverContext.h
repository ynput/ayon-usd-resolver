#ifndef AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H
#define AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H

#include "pluginData/api.h"

#include "pxr/base/vt/value.h"
#include "pxr/pxr.h"
#include "pxr/usd/ar/defineResolverContext.h"
#include "pxr/base/vt/dictionary.h"

#include "cache/resolverContextCache.h"
#include "redirectionFileHanlder/redirectionHanlder.h"

#include <string>

class  AyonUsdResolverContext {
    public:
        // Constructors
        AR_AYONUSDRESOLVER_API
        AyonUsdResolverContext();
        AR_AYONUSDRESOLVER_API
        AyonUsdResolverContext(const AyonUsdResolverContext &ctx);
        AR_AYONUSDRESOLVER_API
        AyonUsdResolverContext(const std::string &configPath);
    
        AR_AYONUSDRESOLVER_API
        ~AyonUsdResolverContext();

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
        // AR_AYONUSDRESOLVER_API
        // void Initialize();
        AR_AYONUSDRESOLVER_API
        void ClearAndReinitialize() const;

        AR_AYONUSDRESOLVER_API
        void dropCache();

        AR_AYONUSDRESOLVER_API
        void deleteFromCache(const std::string &key);

        AR_AYONUSDRESOLVER_API
        void clearCache();
        
        AR_AYONUSDRESOLVER_API
        std::shared_ptr<resolverContextCache> getCachePtr() const;
        
        AR_AYONUSDRESOLVER_API
        const redirectionFile* getRedirectionFile() const;
        
        AR_AYONUSDRESOLVER_API
        void setRedirectionFile(const std::string &rdfPath) const;

    private:
        mutable std::shared_ptr<resolverContextCache> cache;
        ArResolvedPath rootPath;
        mutable std::optional<redirectionFile*> m_redirectionFile;
};

PXR_NAMESPACE_OPEN_SCOPE
AR_DECLARE_RESOLVER_CONTEXT(AyonUsdResolverContext);
PXR_NAMESPACE_CLOSE_SCOPE

#endif   // AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H
