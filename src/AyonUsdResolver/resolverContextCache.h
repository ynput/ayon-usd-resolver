#pragma once

#include <cstddef>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pxr/usd/ar/resolvedPath.h"
#include "AyonCppApi.h"
#include "assetIdentDef.h"

PXR_NAMESPACE_USING_DIRECTIVE

enum cacheName { AYONCACHE, COMMONCACHE };

/**
 * @class resolverContextCache
 * @brief this class handles everything related to asset caching
 *
 */
class resolverContextCache {
    public:
        resolverContextCache();
        resolverContextCache(const size_t &preCacheSize);
        ~resolverContextCache();

        /**
         * @brief move the pair into the precache by using the move operator it will also check if there
         * is enough space in the precache and move the precache if needed
         *
         * @param sourcePair the data that you want to add to the cache as an std::pair
         */
        void insert(std::pair<std::string, pxr::ArResolvedPath> &&sourcePair);

        /**
         * @brief insert a vector into the cache by using the move constructor
         * if the vector is too big to fit into the preCache then the vector will be moved into the ayon cache without
         * touching the precache
         *
         * @param sourceVec the data you want to move into the cache
         */
        void insert(std::vector<std::pair<std::string, pxr::ArResolvedPath>> sourceVec);

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
        void printCache();

    private:
        std::unique_ptr<std::unordered_map<std::string, pxr::ArResolvedPath>> AyonCache;
        std::unique_ptr<std::unordered_map<std::string, pxr::ArResolvedPath>> CommonCache;

        std::unique_ptr<std::unordered_map<std::string, pxr::ArResolvedPath>> PreCache;
        size_t PreCacheFreeItemSlots;
        std::shared_mutex AyonCachesharedMutex;
        std::shared_mutex CommonCachesharedMutex;
        std::shared_mutex PreCachesharedMutex;

        AyonApi ayon;
};
