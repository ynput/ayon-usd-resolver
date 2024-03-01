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
#include <memory>
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
    AyonCache = std::make_unique<std::unordered_map<std::string, pxr::ArResolvedPath>>();
    CommonCache = std::make_unique<std::unordered_map<std::string, pxr::ArResolvedPath>>();
    PreCache = std::make_unique<std::unordered_map<std::string, pxr::ArResolvedPath>>();

    PreCacheFreeItemSlots = 128;
    PreCache->reserve(PreCacheFreeItemSlots);
};
resolverContextCache::resolverContextCache(const size_t &preCacheSize) {
    PreCache->reserve(preCacheSize);
    PreCacheFreeItemSlots = preCacheSize;
};

resolverContextCache::~resolverContextCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::~resolverContextCache \n");
};

// TODO when ayonLogger.h has the header guards then we can import it and use logging from there
void
resolverContextCache::printCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::printCache \n");
    std::cout << "Printing out the Cache Entrys \n" << std::endl;
    std::cout << "PreCache size: " << PreCache->size() << "\n";
    for (const auto &item: *PreCache) {
        std::cout << item.first.c_str() << " // " << item.second.GetPathString().c_str() << "\n";
    }
    std::cout << "AyonCache size: " << AyonCache->size() << "\n";
    for (const auto &item: *AyonCache) {
        std::cout << item.first.c_str() << " // " << item.second.GetPathString().c_str() << "\n";
    }
    std::cout << "CommonCache size: " << CommonCache->size() << "\n";
    for (const auto &item: *CommonCache) {
        std::cout << item.first.c_str() << " // " << item.second.GetPathString().c_str() << "\n";
    }
    std::cout << "-----------------------------------------------------\n" << std::endl;
};

void
resolverContextCache::insert(std::pair<std::string, pxr::ArResolvedPath> &&sourcePair) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::insert \n");

    if (PreCacheFreeItemSlots < 1) {
        AyonCache->reserve(AyonCache->size() + PreCache->size());
        migratePreCacheIntoAyonCache();
        PreCacheFreeItemSlots = PreCache->size();
        PreCache->clear();
    }

    PreCache->insert(std::move(sourcePair));
    PreCacheFreeItemSlots--;
};

// TODO implemnt vector insert
void resolverContextCache::insert(std::vector<std::pair<std::string, pxr::ArResolvedPath>> sourceVec){};

void
resolverContextCache::migratePreCacheIntoAyonCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::migratePreCacheIntoAyonCache \n");
    AyonCache->insert(std::make_move_iterator(PreCache->begin()), std::make_move_iterator(PreCache->end()));
};

ArResolvedPath
resolverContextCache::Find(const std::string &key) const {
    auto hit = PreCache->find(key);
    if (hit != PreCache->end()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
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
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::getAsset: (%s) \n", assetIdentifier.c_str());

    assetIdent asset;
    if (assetIdentifier.empty()) {
        return asset;
    }

    std::unordered_map<std::string, pxr::ArResolvedPath>::iterator hit;

    hit = PreCache->find(assetIdentifier);

    if (hit != PreCache->end()) {
        asset.assetIdentifier = hit->first;
        asset.resolvedAssetPath = hit->second;
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::getAsset: PreCache Hit on (%s) with (%s) \n", asset.assetIdentifier.c_str(),
                 asset.resolvedAssetPath.GetPathString().c_str());
        return asset;
    }
    else {
        switch (selectedCache) {
            case AYONCACHE:
                hit = AyonCache->find(assetIdentifier);
                if (hit != AyonCache->end()) {
                    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                        .Msg("resolverContextCache::getAsset: AyonCache Hit on (%s) with (%s) \n",
                             asset.assetIdentifier.c_str(), asset.resolvedAssetPath.GetPathString().c_str());
                    asset.assetIdentifier = hit->first;
                    asset.resolvedAssetPath = hit->second;
                    return asset;
                }

            case COMMONCACHE:
                hit = CommonCache->find(assetIdentifier);
                if (hit != CommonCache->end()) {
                    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                        .Msg("resolverContextCache::getAsset: CommonCache Hit on (%s) with (%s) \n",
                             asset.assetIdentifier.c_str(), asset.resolvedAssetPath.GetPathString().c_str());
                    asset.assetIdentifier = hit->first;
                    asset.resolvedAssetPath = hit->second;
                    return asset;
                }
        }
    }
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::getAsset: No Cache Hit \n");
    if (isAyonPath) {
        std::pair<std::string, std::string> resolvedAsset = ayon.resolvePath(assetIdentifier);

        asset.assetIdentifier = std::move(resolvedAsset.first);
        asset.resolvedAssetPath = ArResolvedPath(resolvedAsset.second);
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::getAsset: called ayon.resolvePath() \n");
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

            CommonCache->insert(std::make_pair(asset.assetIdentifier, asset.resolvedAssetPath));
        }
    }

    return asset;
};

void
resolverContextCache::removeCachedObject(const std::string &key) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::removeCachedObject (%s) \n", key.c_str());

    std::unordered_map<std::string, pxr::ArResolvedPath>::iterator hit;

    hit = PreCache->find(key);
    if (hit != PreCache->end()) {
        PreCache->erase(hit);
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::removeCachedObject removed object from PreCache");
        return;
    }

    hit = AyonCache->find(key);
    if (hit != AyonCache->end()) {
        AyonCache->erase(hit);
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::removeCachedObject removed object from AyonCache");
        return;
    }

    hit = CommonCache->find(key);
    if (hit != CommonCache->end()) {
        CommonCache->erase(hit);
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::removeCachedObject removed object from CommonCache");
        return;
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("resolverContextCache::removeCachedObject the object could not be found");
};

void
resolverContextCache::removeCachedObject(const std::string &key, const cacheName &selectedCache) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::removeCachedObject (%s) \n", key.c_str());

    std::unordered_map<std::string, pxr::ArResolvedPath>::iterator hit;

    hit = PreCache->find(key);
    if (hit != PreCache->end()) {
        PreCache->erase(hit);
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::removeCachedObject removed object from PreCache");
        return;
    }

    if (hit != PreCache->end()) {
        PreCache->erase(hit);
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::removeCachedObject removed object from PreCache");
        return;
    }
    else {
        switch (selectedCache) {
            case AYONCACHE:
                hit = AyonCache->find(key);
                if (hit != AyonCache->end()) {
                    PreCache->erase(hit);
                    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                        .Msg("resolverContextCache::removeCachedObject removed object from PreCache");
                    return;
                }

            case COMMONCACHE:
                hit = CommonCache->find(key);
                if (hit != CommonCache->end()) {
                    PreCache->erase(hit);
                    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                        .Msg("resolverContextCache::removeCachedObject removed object from PreCache");
                    return;
                }
        }
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("resolverContextCache::removeCachedObject the object could not be found");
};

void
resolverContextCache::clearCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::clearCache \n");
    CommonCache->clear();
    AyonCache->clear();
    PreCache->clear();
};
