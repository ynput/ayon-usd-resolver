#include "resolverContextCache.h"
#include "../config.h"
#include "../helpers/ayonApiGet.h"
#include "../codes/debugCodes.h"
#include "nlohmann/json_fwd.hpp"
#include "../helpers/resolutionFunctions.h"

#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/pathUtils.h"

#include <map>
#include <memory>
#include <ynput/core/iostd/envVarHelpers.hpp>
#include <ynput/tool/ayon/rootHelpers.hpp>

#include <stdexcept>
#include <unordered_map>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE
// TODO pinning file hanlder should construct its cache directly at construction getAssetData should not call
// rootReplace
PinningFileHandler::PinningFileHandler(const std::string &pinningFilePath,
                                       const std::unordered_map<std::string, std::string> &rootReplaceData):
    m_pinningFilePath(pinningFilePath),
    m_rootReplaceData(rootReplaceData) {
    std::ifstream pinningFile(this->m_pinningFilePath);

    if (!pinningFile.is_open()) {
        throw std::runtime_error("PinningFileHandler was not able to open PinningFile: "
                                 + this->m_pinningFilePath.string());
    }

    nlohmann::json raw_pinning_file;
    try {
        raw_pinning_file = nlohmann::json::parse(pinningFile);
    }
    catch (const nlohmann::json::parse_error &e) {
        throw std::runtime_error("The pining File is not in the Correct Format: ");
    }

    nlohmann::json pinningData = raw_pinning_file.at("ayon_resolver_pinning_data");
    pinningData.erase("ayon_pinning_data_entry_scene");

    for (auto &entry: pinningData.items()) {
        std::string pathed_key = ynput::tool::ayon::rootReplace(entry.key(), this->m_rootReplaceData);
        std::string pathed_val = ynput::tool::ayon::rootReplace(entry.value(), this->m_rootReplaceData);
        this->m_pinningFileData[pathed_key] = pathed_val;
    }
};

/**
 * @brief return AssetIdentifier populated with root rootReplaceData from the pinning file using the pinning file data loaded
 * at construction and the PROJECT_ROOTS env variable.
 * this is not a cached function it will reconstruct the AssetIdentifier. it will not reload the file or the env var however.
 *
 * @param resolveKey UsdAssetIdent
 * @return populated AssetIdentifier if key was found in pinning file. Empty AssetIdentifier if key was not found
 */
AssetIdentifier
PinningFileHandler::getAssetData(const std::string &resolveKey) {
    AssetIdentifier assetEntry;

    std::string pinnedAssetPath;
    try {
        pinnedAssetPath = this->m_pinningFileData.at(resolveKey);
    }
    catch (const nlohmann::json::out_of_range &e) {
        return assetEntry;
    }

    if (!pinnedAssetPath.empty()) {
        assetEntry.setAssetIdentifier(resolveKey);
        assetEntry.setResolvedAssetPath(pinnedAssetPath);
    }

    return assetEntry;
};

ResolverContextCache::ResolverContextCache(): m_AyonCache(), m_CommonCache(), m_PreCache(), m_staticCache(true) {
    m_PreCache.reserve(PRECACHE_SIZE);
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::ResolverContextCache() \n");

    const char* enable_static_env_var = std::getenv(ENABLE_STATIC_GLOBAL_CACHE_ENV_KEY);
    if (enable_static_env_var == nullptr || std::strcmp(enable_static_env_var, "false") == 0) {
        std::cout << "Initializing dynamic ResolverContextCache" << std::endl;
        std::unique_ptr<AyonApi> api = getAyonApiFromEnv();
        std::cout << "AyonApi instance created inside ResolverContextCache" << std::endl;
        m_ayon.emplace(std::move(api));

        this->m_staticCache = false;
    }
    else {
        std::map<std::string, std::string> projectRootsEnvMap = ynput::core::iostd::getEnvMap(PROJECT_ROOTS_ENV_KEY);
        std::unordered_map<std::string, std::string> projectRootsEnvUMap(
            std::make_move_iterator(projectRootsEnvMap.begin()), std::make_move_iterator(projectRootsEnvMap.end()));
        this->m_pinningFileHandler.emplace(ynput::core::iostd::getEnvKey(PINNING_FILE_PATH_ENV_KEY),
                                           projectRootsEnvUMap);
    }
};

ResolverContextCache::~ResolverContextCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::~ResolverContextCache() \n");
};

// TODO when ayonLogger.h has the header guards then we can import it and use logging from there
void
ResolverContextCache::printCache() const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::printCache \n");

    std::shared_lock<std::shared_mutex> PreCacheReadLock(this->m_PreCacheSharedMutex);
    std::shared_lock<std::shared_mutex> AyonCacheReadLock(this->m_AyonCacheSharedMutex);
    std::shared_lock<std::shared_mutex> CommonCacheReadLock(this->m_CommonCacheSharedMutex);
    std::cout << "Printing out the Cache Entries \n";

    std::cout << "PreCache size: " << this->m_PreCache.size() << "\n";
    for (const auto &assetIdentifierInstance: this->m_PreCache) {
        assetIdentifierInstance.printInfo();
    }
    std::cout << "AyonCache size: " << m_AyonCache.size() << "\n";
    for (const auto &assetIdentifierInstance: m_AyonCache) {
        assetIdentifierInstance.printInfo();
    }
    std::cout << "CommonCache size: " << m_CommonCache.size() << "\n";
    for (const auto &assetIdentifierInstance: m_CommonCache) {
        assetIdentifierInstance.printInfo();
    }
    std::ostringstream oss;
    oss << static_cast<const void*>(this);
    std::cout << "ResolverContextCache infos;" << " Instance_M_Pose; " << oss.str().c_str() << " Instance_m_Size; "
              << std::to_string(sizeof(*this)).c_str() << "\n";

    std::cout << "-----------------------------------------------------\n" << std::endl;
};

void
ResolverContextCache::insert(AssetIdentifier &sourceAssetIdent) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("ResolverContextCache::insert(%s) \n", sourceAssetIdent.getAssetIdentifier().c_str());
    if (m_PreCache.size() == PRECACHE_SIZE) {
        migratePreCacheIntoAyonCache();
    }

    std::unique_lock<std::shared_mutex> PreCacheWriteLock(this->m_PreCacheSharedMutex);
    std::unique_lock<std::shared_mutex> AyonCacheWriteLock(this->m_AyonCacheSharedMutex);

    this->m_PreCache.insert(std::move(sourceAssetIdent));
};

void
ResolverContextCache::migratePreCacheIntoAyonCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::migratePreCacheIntoAyonCache \n");
    std::unique_lock<std::shared_mutex> PreCacheWriteLock(this->m_PreCacheSharedMutex);
    std::unique_lock<std::shared_mutex> AyonCacheWriteLock(this->m_AyonCacheSharedMutex);

    m_AyonCache.reserve(m_AyonCache.size() + m_PreCache.size());
    m_AyonCache.insert(std::make_move_iterator(m_PreCache.begin()), std::make_move_iterator(m_PreCache.end()));
    m_PreCache.clear();
};

AssetIdentifier
ResolverContextCache::getAsset(const std::string &assetIdentifier,
                               const CacheName selectedCache,
                               const bool isAyonPath) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::getAsset: (%s) \n", assetIdentifier.c_str());

    AssetIdentifier asset;

    if (assetIdentifier.empty()) {
        return asset;
    }
    if (this->m_staticCache) {
        return this->m_pinningFileHandler->getAssetData(assetIdentifier);
    }

    std::unordered_set<AssetIdentifier, AssetIdentifierHash>::iterator hit;

    std::shared_lock<std::shared_mutex> preCacheSharedLock(m_PreCacheSharedMutex);
    hit = m_PreCache.find(assetIdentifier);
    if (hit != m_PreCache.end()) {
        asset = *hit;
        preCacheSharedLock.unlock();

        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::getAsset: PreCache Hit on (%s) with (%s) \n",
                 asset.getAssetIdentifier().c_str(), asset.getResolvedAssetPath().GetPathString().c_str());
        return asset;
    }
    preCacheSharedLock.unlock();

    std::cout << "[resolver context] No PreCache hit for assetIdentifier: " << assetIdentifier << std::endl;

    switch (selectedCache) {
        case CacheName::AYONCACHE:
            {
                std::shared_lock<std::shared_mutex> ayonCacheSharedLock(m_AyonCacheSharedMutex);
                hit = m_AyonCache.find(assetIdentifier);
                if (hit != m_AyonCache.end()) {
                    asset = *hit;
                    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::getAsset: AyonCache Hit \n");
                }

                ayonCacheSharedLock.unlock();
                break;
            }

        case CacheName::COMMONCACHE:
            {
                std::shared_lock<std::shared_mutex> CommonCacheSharedLock(m_CommonCacheSharedMutex);
                hit = m_CommonCache.find(assetIdentifier);
                if (hit != m_CommonCache.end()) {
                    asset = *hit;
                    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                        .Msg("ResolverContextCache::getAsset: CommonCache Hit \n");
                }

                CommonCacheSharedLock.unlock();
                break;
            }
    }
    if (!asset.is_empty()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::getAsset: Cache Hit with (%s) with (%s) \n", asset.getAssetIdentifier().c_str(),
                 asset.getResolvedAssetPath().GetPathString().c_str());
        return asset;
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::getAsset: No Cache Hit \n");
    if (isAyonPath) {
        std::cout << "[resolver context] Get from server: " << assetIdentifier << std::endl;
        std::pair<std::string, std::string> resolvedAsset = m_ayon->get()->resolvePath(assetIdentifier);


        asset.setAssetIdentifier(std::move(resolvedAsset.first));
        asset.setResolvedAssetPath(std::move(resolvedAsset.second));

        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::getAsset: called ayon.resolvePath() \n");
        this->insert(asset);
    }
    else {
        if (_IsRelativePath(assetIdentifier)) {
            asset.setResolvedAssetPath(_ResolveAnchored(ArchGetCwd(), assetIdentifier));
        }
        else {
            asset.setResolvedAssetPath(ArResolvedPath(TfNormPath(TfAbsPath(assetIdentifier))));
        }
        if (!asset.getResolvedAssetPath().empty()) {
            asset.setAssetIdentifier(assetIdentifier);

            std::shared_lock<std::shared_mutex> CommonCacheSharedLock(m_CommonCacheSharedMutex);

            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("ResolverContextCache::getAsset: insert into CommonCache \n");
            m_CommonCache.insert(asset);
        }
    }

    return asset;
};

void
ResolverContextCache::removeCachedObject(const std::string &key) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::removeCachedObject (%s) \n", key.c_str());

    std::unordered_set<AssetIdentifier>::iterator hit;

    std::unique_lock<std::shared_mutex> preCacheSharedWriteLock(m_PreCacheSharedMutex);

    hit = m_PreCache.find(key);
    if (hit != m_PreCache.end()) {
        m_PreCache.erase(hit);
        preCacheSharedWriteLock.unlock();
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::removeCachedObject removed object from PreCache");
        return;
    }

    std::unique_lock<std::shared_mutex> AyonCachesharedWriteLock(m_AyonCacheSharedMutex);

    hit = m_AyonCache.find(key);
    if (hit != m_AyonCache.end()) {
        m_AyonCache.erase(hit);
        AyonCachesharedWriteLock.unlock();
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::removeCachedObject removed object from AyonCache");
        return;
    }

    std::unique_lock<std::shared_mutex> CommonCachesharedWriteLock(m_CommonCacheSharedMutex);
    hit = m_CommonCache.find(key);
    if (hit != m_CommonCache.end()) {
        m_CommonCache.erase(hit);
        CommonCachesharedWriteLock.unlock();
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::removeCachedObject removed object from CommonCache");
        return;
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("ResolverContextCache::removeCachedObject the object could not be found");
};

void
ResolverContextCache::removeCachedObject(const std::string &key, const CacheName selectedCache) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::removeCachedObject (%s) \n", key.c_str());

    std::unordered_set<AssetIdentifier>::iterator hit;

    std::unique_lock<std::shared_mutex> preCacheSharedDeleteLock(m_PreCacheSharedMutex);
    hit = m_PreCache.find(key);
    if (hit != m_PreCache.end()) {
        m_PreCache.erase(hit);
        preCacheSharedDeleteLock.unlock();
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::removeCachedObject removed object from PreCache");

        return;
    }
    else {
        switch (selectedCache) {
            case CacheName::AYONCACHE:
                {
                    std::unique_lock<std::shared_mutex> AyonCachesharedDellLock(m_AyonCacheSharedMutex);

                    hit = m_AyonCache.find(key);
                    if (hit != m_AyonCache.end()) {
                        m_AyonCache.erase(hit);
                        AyonCachesharedDellLock.unlock();
                        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                            .Msg("ResolverContextCache::removeCachedObject removed object from AyonCache");
                        return;
                    }
                    break;
                }
            case CacheName::COMMONCACHE:
                {
                    std::unique_lock<std::shared_mutex> CommonCachesharedDellLock(m_CommonCacheSharedMutex);
                    hit = m_CommonCache.find(key);
                    if (hit != m_CommonCache.end()) {
                        m_CommonCache.erase(hit);
                        CommonCachesharedDellLock.unlock();
                        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                            .Msg("ResolverContextCache::removeCachedObject removed object from CommonCache");
                        return;
                    }
                    break;
                }
        }
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("ResolverContextCache::removeCachedObject the object could not be found");
};

void
ResolverContextCache::clearCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::clearCache \n");

    std::unique_lock<std::shared_mutex> PreCachesharedMutexLock(m_PreCacheSharedMutex);
    std::unique_lock<std::shared_mutex> AyonCachesharedMutexLock(m_AyonCacheSharedMutex);
    std::unique_lock<std::shared_mutex> CommonCachesharedMutexLock(m_CommonCacheSharedMutex);
    m_CommonCache.clear();
    m_AyonCache.clear();
    m_PreCache.clear();
};

bool
ResolverContextCache::isCacheStatic() const {
    return this->m_staticCache;
};
