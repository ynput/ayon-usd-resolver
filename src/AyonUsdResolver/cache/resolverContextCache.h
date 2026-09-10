/**
 * @file resolverContextCache.h
 * @brief Declaration of the ResolverContextCache and PinningFileHandler classes.
 */

#ifndef AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_CACHE_H
#define AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_CACHE_H

#include "../cache/assetIdentifierDef.h"
#include "memcachedHandler.h"
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

/**
 * @brief Convert a resolved path to a rootless path using the provided root replacement data.
 *
 * @param resolvedPath The resolved path to convert.
 * @param rootReplaceData A map of root paths to their replacements.
 * @return The rootless path.
 */

static std::string _ToRootlessPath(
    const std::string &resolvedPath,
    const std::unordered_map<std::string,
    std::string> &rootReplaceData);

/**
 * @brief Construct a new Pinning File Handler object and load the pinning file data.
 *
 * @param pinningFilePath Path to the pinning file.
 * @param rootReplaceData Root replacement data to apply to the pinning file entries.
 * @todo pinning file handlder should construct its cache directly at construction getAssetData should not call
 *       rootReplace
 */
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
         * @brief Resolve many AYON URIs in a single batched (parallel) request and seed the cache.
         *
         * Unlike getAsset(), which resolves one URI per server round-trip, this collapses a whole
         * set of URIs into one batched call via AyonApi::batchResolvePath and inserts every result.
         * Used by the prewarm pass to avoid the serial resolve storm during stage composition.
         *
         * No-op in static (pinning) mode or with an empty input.
         * @param uriPaths The AYON URIs to resolve. May be reordered/deduplicated.
         * @return Map of URI -> resolved path for the entries that were resolved.
         */
        std::unordered_map<std::string, std::string> batchWarm(std::vector<std::string> &uriPaths);

        /**
         * @brief Resolved path for a URI already held by an in-process cache, if any.
         *
         * Lets batchWarm() skip a memcached round trip for URIs PreCache/AyonCache/CommonCache
         * can already answer. Returns the path (so the prewarm BFS can still descend through
         * the layer) rather than a bare bool.
         */
        std::optional<std::string> inProcessResolved(const std::string &uriPath) const;

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
        void ClearCache();

        /**
         * @brief Print the contents of the Resolver Context Cache
         * 
         * @note This function is primarily for debugging purposes and prints the cache contents to the standard output.
         * @todo when ayonLogger.h (in ayon-cpp-dev-tools) has the header guards then we can import
         * it and use logging from there.
         * 
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

        std::unordered_map<std::string, std::string> m_rootReplaceData;
        std::optional<PinningFileHandler> m_pinningFileHandler;
        std::optional<std::unique_ptr<MemcachedHandler>> m_memcached;
};

#endif   // AR_AYONUSDRESOLVER_RESOLVER_CONTEXT_CACHE_H
