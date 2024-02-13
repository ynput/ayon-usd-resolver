#include "resolverContextCache.h"
#include "debugCodes.h"
#include "reseutionFunctions.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE

bool
assetIdent::empty() {
    if (assetIdentifier.empty() && resolvedAssetPath.empty()) {
        return true;
    }

    return false;
};

resolverContextCache::resolverContextCache() {
    PreCacheFreeItemSlots = 128;
    PreCache.reserve(PreCacheFreeItemSlots);
};
resolverContextCache::resolverContextCache(const size_t &preCacheSize) {
    PreCache.reserve(preCacheSize);
    PreCacheFreeItemSlots = preCacheSize;
};

resolverContextCache::~resolverContextCache(){};

void
resolverContextCache::insert(std::pair<std::string, pxr::ArResolvedPath> &&sourcePair) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("resolverContextCache::insert \n");

    if (PreCacheFreeItemSlots < 1) {
        AyonCache.reserve(AyonCache.size() + PreCache.size());
        migratePreCacheIntoAyonCache();
        PreCacheFreeItemSlots = PreCache.size();
        PreCache.clear();
    }

    PreCache.insert(std::move(sourcePair));
    PreCacheFreeItemSlots--;
};

// TODO implemnt vector insert
void resolverContextCache::insert(std::vector<std::pair<std::string, pxr::ArResolvedPath>> sourceVec){};

void
resolverContextCache::migratePreCacheIntoAyonCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("resolverContextCache::migratePreCacheIntoAyonCache \n");
    AyonCache.insert(std::make_move_iterator(PreCache.begin()), std::make_move_iterator(PreCache.end()));
};

ArResolvedPath
resolverContextCache::Find(const std::string &key) const {
    auto hit = PreCache.find(key);
    if (hit != PreCache.end()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
            .Msg("resolverContextCache::Find cache hit (key: %s // path: %s) \n", hit->first.c_str(),
                 hit->second.GetPathString().c_str());
        return hit->second;
    }

    return pxr::ArResolvedPath();
};

assetIdent
resolverContextCache::getAsset(const std::string &assetIdentifier,
                               const cacheName &selectedCache,
                               const bool &isAyonPath) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("resolverContextCache::getAsset: (%s) \n", assetIdentifier.c_str());

    assetIdent asset;
    if (assetIdentifier.empty()) {
        return asset;
    }

    std::unordered_map<std::string, pxr::ArResolvedPath>::iterator hit;

    hit = PreCache.find(assetIdentifier);

    if (hit != PreCache.end()) {
        asset.assetIdentifier = hit->first;
        asset.resolvedAssetPath = hit->second;
        return asset;
    }
    else {
        switch (selectedCache) {
            case AYONCACHE:
                hit = AyonCache.find(assetIdentifier);
                if (hit != AyonCache.end()) {
                    asset.assetIdentifier = hit->first;
                    asset.resolvedAssetPath = hit->second;
                    return asset;
                }

            case COMMONCACHE:
                hit = CommonCache.find(assetIdentifier);
                if (hit != CommonCache.end()) {
                    asset.assetIdentifier = hit->first;
                    asset.resolvedAssetPath = hit->second;
                    return asset;
                }
        }
    }
    if (isAyonPath) {
        std::pair<std::string, std::string> resolvedAsset = ayon.resolvePath(assetIdentifier);
        asset.assetIdentifier = std::move(resolvedAsset.first);
        asset.resolvedAssetPath = ArResolvedPath(resolvedAsset.second);

        insert(std::make_pair(asset.assetIdentifier, asset.resolvedAssetPath));
    }
    else {
        if (_IsRelativePath(assetIdentifier)) {
            asset.resolvedAssetPath = _ResolveAnchored(ArchGetCwd(), assetIdentifier);
        }
        else {
            asset.resolvedAssetPath = ArResolvedPath(TfNormPath(TfAbsPath(assetIdentifier)));
        }
        if (!asset.resolvedAssetPath.empty()) {
            asset.assetIdentifier = assetIdentifier;

            CommonCache.insert(std::make_pair(asset.assetIdentifier, asset.resolvedAssetPath));
        }
    }

    return asset;
};
