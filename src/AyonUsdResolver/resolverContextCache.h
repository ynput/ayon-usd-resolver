#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "pxr/usd/ar/resolvedPath.h"

PXR_NAMESPACE_USING_DIRECTIVE

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
        /**
         * @brief this function tests if the precache can store the data you want to add
         * if it cant store the data it will move the PreCache into the AyonCache
         *
         * @return true if you can add data
         */
        bool preCacheCheck(const size_t &newEntries);

        // TODO change this function to write out to an asset path and to be an bool function
        /**
         * @brief this function returns the ArResolvedPath for the given assetIdentifier
         * if the function is unable to find the assetIdentifier then it returns an empty ArResolvedPath
         *
         * @param key
         */
        ArResolvedPath Find(const std::string &key) const;

    private:
        std::unordered_map<std::string, pxr::ArResolvedPath> AyonCache;
        std::unordered_map<std::string, pxr::ArResolvedPath> CommonCache;

        std::unordered_map<std::string, pxr::ArResolvedPath> PreCache;
        size_t PreCacheSize;
};
