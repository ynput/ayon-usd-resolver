#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pxr/usd/ar/resolvedPath.h"
#include "AyonCppApi.h"

PXR_NAMESPACE_USING_DIRECTIVE

struct assetIdent {
        ArResolvedPath resolvedAssetPath;
        std::string assetIdentifier;
        bool empty();
};

enum cacheName { AYONCACHE, COMMONCACHE };

class resolverContextCache {
    public:
        resolverContextCache();
        resolverContextCache(const size_t &preCacheSize);
        ~resolverContextCache();

        /**
         * @brief this function moves the pair into the precache by using the move operator it will also check if there
         * is enought space in the precache and move the precache if needed
         *
         * @param sourcePair the data that you want to add to the cache as an std::pair
         */
        void insert(std::pair<std::string, pxr::ArResolvedPath> &&sourcePair);
        /**
         * @brief inserts a vector into the cache by using the move constructor
         * if the vector is to big to fit into the preCache then the vector will be moved ito the ayon cache without
         * touchgin the precache
         *
         * @param sourceVec the data you want to move into the cache
         */
        void insert(std::vector<std::pair<std::string, pxr::ArResolvedPath>> sourceVec);
        /**
         * @brief moves the precache into the AyonCache in order to free the precache
         */
        void migratePreCacheIntoAyonCache();

        // TODO change this function to write out to an asset path and to be an bool function
        /**
         * @brief this function returns the ArResolvedPath for the given assetIdentifier
         * if the function is unable to find the assetIdentifier then it returns an empty ArResolvedPath
         *
         * @param key
         */
        ArResolvedPath Find(const std::string &key) const;

        /**
         * @brief returns a struct by first searching true the selected cacheName if no cache hit. then the function
         * will resolve the path against ayon if even that dosnt work it will return an empty path
         *
         * @param assetIdentifier
         * @return
         */
        assetIdent getAsset(const std::string &assetIdentifier, const cacheName &selectedCache, const bool &isAyonPath);

        /**
         * @brief this function sets up the cache from a pinning file
         *
         * @param pinningFilePath
         */
        void setCacheFromPinningFile(const std::string &pinningFilePath);

    private:
        std::unordered_map<std::string, pxr::ArResolvedPath> AyonCache;
        std::unordered_map<std::string, pxr::ArResolvedPath> CommonCache;

        std::unordered_map<std::string, pxr::ArResolvedPath> PreCache;
        size_t PreCacheFreeItemSlots;

        AyonApi ayon;
};
