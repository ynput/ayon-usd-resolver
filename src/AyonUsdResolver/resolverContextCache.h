#ifndef AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_CACHE_H
#define AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_CACHE_H

#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_set>

#include "AyonCppApi.h"
#include "assetIdentDef.h"
#include <nlohmann/json.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

enum cacheName { AYONCACHE, COMMONCACHE };

class pinningFileHanlder {
    public:
        pinningFileHanlder(const std::string &pinningFilePath);
        ~pinningFileHanlder() = default;

        assetIdent getAssetData(const std::string &resolveKey);

    private:
        std::filesystem::path m_pinningFilePath;
        nlohmann::json m_pinningFileData;
};

/**
 * @class resolverContextCache
 * @brief this class handles everything related to asset caching
 *
 */
class resolverContextCache {
    public:
        resolverContextCache();
        ~resolverContextCache();

        /**
         * @brief move the pair into the preCache by using the move operator it will also check if there
         * is enough space in the preCache and move the preCache if needed. This function is both locking and blocking
         * so no access will be granted to ayonCache or preCache for the scope of this function
         *
         * @param sourcePair the data that you want to add to the cache as an std::pair
         */
        void insert(assetIdent &sourceAssetIdent);

        /**
         * @brief move the precache into the AyonCache in order to free the precache
         *
         */
        void migratePreCacheIntoAyonCache();

        /**
         * @brief return a struct by first searching through the selected cacheName if no cache hit. then the function
         * will resolve the path against ayon if even that doesn't work it will return an empty path
         *
         * @param assetIdentifier
         * @return
         */
        assetIdent getAsset(const std::string &assetIdentifier, const cacheName &selectedCache, const bool &isAyonPath);

        /**
         * @brief set up the cache from a pinning file
         *
         * @param pinningFilePath
         */
        void setCacheFromPinningFile(const std::string &pinningFilePath);

        /**
         * @brief this function allows the deletion of an entry in the cache
         *
         * @param key the asset identifier / uri of the usd object.
         */
        void removeCachedObject(const std::string &key);

        /**
         * @brief delete an entry in a selected cache. the PreCache will always we
         * searched for the entry.
         *
         * @param key the asset identifier / uri of the usd object.
         * @param selectedCache enum that allows you to select the cache (cacheName enum)
         */
        void removeCachedObject(const std::string &key, const cacheName &selectedCache);

        /**
         * @brief clear the complete cache
         */
        void clearCache();

        /**
         * @brief print out every object in the cache for debugging
         */
        void printCache() const;

    private:
        std::unordered_set<assetIdent, assetIdentHash> m_PreCache;
        std::unordered_set<assetIdent, assetIdentHash> m_AyonCache;
        std::unordered_set<assetIdent, assetIdentHash> m_CommonCache;

        mutable std::shared_mutex m_PreCachesharedMutex;
        mutable std::shared_mutex m_AyonCachesharedMutex;
        mutable std::shared_mutex m_CommonCachesharedMutex;

        std::optional<AyonApi> m_ayon;
        bool m_static_cache;

        std::optional<pinningFileHanlder> m_pinningFileHanlder;
};

#endif   // AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_CACHE_H
