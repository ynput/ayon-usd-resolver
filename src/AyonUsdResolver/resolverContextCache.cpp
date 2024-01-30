#include "resolverContextCache.h"
#include "pxr/usd/ar/resolvedPath.h"
#include <cstddef>
#include <string>
#include <utility>

#include "debugCodes.h"

PXR_NAMESPACE_USING_DIRECTIVE

resolverContextCache::resolverContextCache() {
    PreCache.reserve(128);   // reserving a defined stoarge field so taht we can avoide heap allocations
    PreCacheSize = 128;
    PreCache.insert(std::make_pair("ayonx:", pxr::ArResolvedPath("/home/workh/Ynput/dev/dell/gg.usd")));
};
resolverContextCache::resolverContextCache(const size_t &preCacheSize) {
    PreCache.reserve(preCacheSize);
    PreCacheSize = preCacheSize;
};

resolverContextCache::~resolverContextCache(){};

void
resolverContextCache::insert(std::pair<std::string, pxr::ArResolvedPath> &&sourcePair) {
    PreCache.insert(std::move(sourcePair));
};
void resolverContextCache::insert(std::vector<std::pair<std::string, pxr::ArResolvedPath>> sourceVec){};

bool
resolverContextCache::preCacheCheck(const size_t &newEntries) {
    return true;
};

ArResolvedPath
resolverContextCache::Find(const std::string &key) const {
    auto hit = PreCache.find(key);
    if (hit != PreCache.end()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
            .Msg("cache Found (key: %s // path: %s) \n", hit->first.c_str(), hit->second.GetPathString().c_str());
        return hit->second;
    }
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("no cache was found");

    return pxr::ArResolvedPath();
};

void resolverContextCache::migratePreCacheIntoAyonCache(){};
