#ifndef AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H
#define AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H

#include "pluginData/api.h"

#include "pxr/pxr.h"
#include "pxr/usd/ar/defineResolverContext.h"

#include "cache/resolverContextCache.h"

#include <string>

class AyonUsdResolverContext {
    public:
        // Constructors
        AR_AYONUSDRESOLVER_API
        AyonUsdResolverContext();
        AR_AYONUSDRESOLVER_API
        AyonUsdResolverContext(const AyonUsdResolverContext &ctx);
        AR_AYONUSDRESOLVER_API
        AyonUsdResolverContext(const std::string &filePath);
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
        AR_AYONUSDRESOLVER_API
        void Initialize();
        AR_AYONUSDRESOLVER_API
        void ClearAndReinitialize();

        AR_AYONUSDRESOLVER_API
        void dropCache();

        AR_AYONUSDRESOLVER_API
        void deleteFromCache(const std::string &key);

        AR_AYONUSDRESOLVER_API
        void clearCache();

        AR_AYONUSDRESOLVER_API
        const std::string& GetMappingFilePath() const;
        AR_AYONUSDRESOLVER_API
        void SetMappingFilePath(std::string filePath);
        AR_AYONUSDRESOLVER_API
        void RefreshFromMappingFilePath();
        AR_AYONUSDRESOLVER_API
        void AddMappingPair(const std::string& sourceStr, const std::string& targetStr);
        AR_AYONUSDRESOLVER_API
        void RemoveMappingByKey(const std::string& sourceStr);
        AR_AYONUSDRESOLVER_API
        void RemoveMappingByValue(const std::string& targetStr);
        AR_AYONUSDRESOLVER_API
        const std::map<std::string, std::string>& GetMappingPairs() const;

        std::shared_ptr<ResolverContextCache> getCachePtr() const;

    private:
        std::shared_ptr<ResolverContextCache> cache;
        ArResolvedPath rootPath;
        std::string mappingFilePath;
        std::unordered_map<std::string, std::string> mappingPairs;

        bool _GetMappingPairsFromUsdFile(const std::string& filePath);
};

PXR_NAMESPACE_OPEN_SCOPE
AR_DECLARE_RESOLVER_CONTEXT(AyonUsdResolverContext);
PXR_NAMESPACE_CLOSE_SCOPE

#endif   // AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_H
