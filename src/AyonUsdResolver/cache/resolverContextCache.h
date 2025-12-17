#ifndef AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_CACHE_H
#define AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_CACHE_H

#include "../cache/assetIdentifierDef.h"
#include "AyonCppApi.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

enum class CacheName { AYONCACHE, COMMONCACHE };

class PinningFileHandler {
    public:
        PinningFileHandler(const std::string &pinningFilePath,
                           const std::unordered_map<std::string, std::string> &rootReplaceData);
        ~PinningFileHandler() = default;

        AssetIdentifier getAssetData(const std::string &resolveKey);

    private:
        std::filesystem::path m_pinningFilePath;
        nlohmann::json m_pinningFileData;

        std::unordered_map<std::string, std::string> m_rootReplaceData;
};


/**
 * @class ResolverContextCache
 * @brief Handles everything related to asset caching
 */
class ResolverContextCache {
    public:
        ResolverContextCache();
        ~ResolverContextCache();

        /**
         * @brief Move the asset into the preCache. Checks space and migrates if needed.
         * This function is both locking and blocking.
         * @param sourceAssetIdent The asset data to add to the cache
         */
        void insert(AssetIdentifier &sourceAssetIdent);

        /**
         * @brief Move the precache into the AyonCache to free the precache
         */
        void migratePreCacheIntoAyonCache();

        /**
         * @brief Return an asset by searching the selected cache. If not found, resolve
         * against AYON. Returns empty path if resolution fails.
         * @param assetIdentifier The asset URI to resolve
         * @param selectedCache Which cache to search first
         * @param isAyonPath Whether this is an AYON URI
         * @return AssetIdentifier with resolved path
         */
        AssetIdentifier getAsset(const std::string &assetIdentifier, const CacheName selectedCache, const bool isAyonPath);

        /**
         * @brief Set up the cache from a pinning file
         * @param pinningFilePath Path to the pinning file
         */
        void setCacheFromPinningFile(const std::string &pinningFilePath);

        /**
         * @brief Delete an entry from the cache
         * @param key The asset identifier/URI
         */
        void removeCachedObject(const std::string &key);

        /**
         * @brief Delete an entry from a selected cache. PreCache is always searched.
         * @param key The asset identifier/URI
         * @param selectedCache Which cache to remove from
         */
        void removeCachedObject(const std::string &key, const CacheName selectedCache);

        /**
         * @brief Clear the complete cache
         */
        void clearCache();

        /**
         * @brief Print every object in the cache for debugging
         */
        void printCache() const;

        /**
         * @brief Check if cache is static (no dynamic resolution)
         */
        bool isCacheStatic() const;

    private:
        std::unordered_set<AssetIdentifier, AssetIdentifierHash> m_PreCache;
        std::unordered_set<AssetIdentifier, AssetIdentifierHash> m_AyonCache;
        std::unordered_set<AssetIdentifier, AssetIdentifierHash> m_CommonCache;

        mutable std::shared_mutex m_PreCacheSharedMutex;
        mutable std::shared_mutex m_AyonCacheSharedMutex;
        mutable std::shared_mutex m_CommonCacheSharedMutex;

        std::optional<std::unique_ptr<AyonApi>> m_ayon;
        bool m_staticCache;

        std::optional<PinningFileHandler> m_pinningFileHandler;
};

#endif   // AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_CACHE_H
