#include "resolverContextCache.h"
#include "../config.h"
#include "../codes/debugCodes.h"
#include "nlohmann/json_fwd.hpp"
#include "../helpers/resolutionFunctions.h"

#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/pathUtils.h"

#include <map>
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
pinningFileHandler::pinningFileHandler(const std::string &pinningFilePath,
                                       const std::unordered_map<std::string, std::string> &rootReplaceData):
    m_pinningFilePath(pinningFilePath),
    m_rootReplaceData(rootReplaceData) {
    std::ifstream pinningFile(this->m_pinningFilePath);

    if (!pinningFile.is_open()) {
        throw std::runtime_error("pinningFileHandler was not able to open PinningFile: "
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
 * @brief return assetIdent populated with root rootReplaceData from the pinning file using the pinning file data loaded
 * at construction and the PROJECT_ROOTS env variable.
 * this is not a cached function it will reconstruct the assetIdent. it will not reload the file or the env var however.
 *
 * @param resolveKey UsdAssetIdent
 * @return populated assetIdent if key was found in pinning file. Empty assetIdent if key was not found
 */
assetIdent
pinningFileHandler::getAssetData(const std::string &resolveKey) {
    assetIdent assetEntry;

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

resolverContextCache::resolverContextCache(): m_AyonCache(), m_CommonCache(), m_PreCache(), m_static_cache(true) {
    m_PreCache.reserve(PRECACHE_SIZE);
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::resolverContextCache() \n");

    const char* enable_static_env_var = std::getenv(ENABLE_STATIC_GLOBAL_CACHE_ENV_KEY);
    if (enable_static_env_var == nullptr || std::strcmp(enable_static_env_var, "false") == 0) {
        m_ayon.emplace();
        this->m_static_cache = false;
    }
    else {
        std::map<std::string, std::string> projectRootsEnvMap = ynput::core::iostd::getEnvMap(PROJECT_ROOTS_ENV_KEY);
        std::unordered_map<std::string, std::string> projectRootsEnvUMap(
            std::make_move_iterator(projectRootsEnvMap.begin()), std::make_move_iterator(projectRootsEnvMap.end()));
        this->m_pinningFileHandler.emplace(ynput::core::iostd::getEnvKey(PINNING_FILE_PATH_ENV_KEY),
                                           projectRootsEnvUMap);
    }
};

resolverContextCache::~resolverContextCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::~resolverContextCache() \n");
};

// TODO when ayonLogger.h has the header guards then we can import it and use logging from there
void
resolverContextCache::printCache() const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::printCache \n");

    std::shared_lock<std::shared_mutex> PreCacheReadLock(this->m_PreCachesharedMutex);
    std::shared_lock<std::shared_mutex> AyonCacheReadLock(this->m_AyonCachesharedMutex);
    std::shared_lock<std::shared_mutex> CommonCacheReadLock(this->m_CommonCachesharedMutex);
    std::cout << "Printing out the Cache Entries \n";

    std::cout << "PreCache size: " << this->m_PreCache.size() << "\n";
    for (const auto &assetIdentInstance: this->m_PreCache) {
        assetIdentInstance.printInfo();
    }
    std::cout << "AyonCache size: " << m_AyonCache.size() << "\n";
    for (const auto &assetIdentInstance: m_AyonCache) {
        assetIdentInstance.printInfo();
    }
    std::cout << "CommonCache size: " << m_CommonCache.size() << "\n";
    for (const auto &assetIdentInstance: m_CommonCache) {
        assetIdentInstance.printInfo();
    }
    std::ostringstream oss;
    oss << static_cast<const void*>(this);
    std::cout << "resolverContextCache infos;" << " Instance_M_Pose; " << oss.str().c_str() << " Instance_m_Size; "
              << std::to_string(sizeof(*this)).c_str() << "\n";

    std::cout << "-----------------------------------------------------\n" << std::endl;
};

void
resolverContextCache::insert(assetIdent &sourceAssetIdent) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("resolverContextCache::insert(%s) \n", sourceAssetIdent.getAssetIdentifier().c_str());
    if (m_PreCache.size() == PRECACHE_SIZE) {
        migratePreCacheIntoAyonCache();
    }

    std::unique_lock<std::shared_mutex> PreCacheWriteLock(this->m_PreCachesharedMutex);
    std::unique_lock<std::shared_mutex> AyonCacheWriteLock(this->m_AyonCachesharedMutex);

    this->m_PreCache.insert(std::move(sourceAssetIdent));
};

void
resolverContextCache::migratePreCacheIntoAyonCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::migratePreCacheIntoAyonCache \n");
    std::unique_lock<std::shared_mutex> PreCacheWriteLock(this->m_PreCachesharedMutex);
    std::unique_lock<std::shared_mutex> AyonCacheWriteLock(this->m_AyonCachesharedMutex);

    m_AyonCache.reserve(m_AyonCache.size() + m_PreCache.size());
    m_AyonCache.insert(std::make_move_iterator(m_PreCache.begin()), std::make_move_iterator(m_PreCache.end()));
    m_PreCache.clear();
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
    if (this->m_static_cache) {
        return this->m_pinningFileHandler->getAssetData(assetIdentifier);
    }

    std::unordered_set<assetIdent, assetIdentHash>::iterator hit;

    std::shared_lock<std::shared_mutex> PreCachesharedLock(m_PreCachesharedMutex);
    hit = m_PreCache.find(assetIdentifier);
    if (hit != m_PreCache.end()) {
        asset = *hit;
        PreCachesharedLock.unlock();

        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::getAsset: PreCache Hit on (%s) with (%s) \n",
                 asset.getAssetIdentifier().c_str(), asset.getResolvedAssetPath().GetPathString().c_str());
        return asset;
    }
    PreCachesharedLock.unlock();

    switch (selectedCache) {
        case AYONCACHE:
            {
                std::shared_lock<std::shared_mutex> AyonCacheSharedLock(m_AyonCachesharedMutex);
                hit = m_AyonCache.find(assetIdentifier);
                if (hit != m_AyonCache.end()) {
                    asset = *hit;
                    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::getAsset: AyonCache Hit \n");
                }

                AyonCacheSharedLock.unlock();
                break;
            }

        case COMMONCACHE:
            {
                std::shared_lock<std::shared_mutex> CommonCacheSharedLock(m_CommonCachesharedMutex);
                hit = m_CommonCache.find(assetIdentifier);
                if (hit != m_CommonCache.end()) {
                    asset = *hit;
                    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                        .Msg("resolverContextCache::getAsset: CommonCache Hit \n");
                }

                CommonCacheSharedLock.unlock();
                break;
            }
    }
    if (!asset.is_empty()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::getAsset: Cache Hit with (%s) with (%s) \n", asset.getAssetIdentifier().c_str(),
                 asset.getResolvedAssetPath().GetPathString().c_str());
        return asset;
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::getAsset: No Cache Hit \n");
    if (isAyonPath) {
        std::pair<std::string, std::string> resolvedAsset = m_ayon->resolvePath(assetIdentifier);
        asset.setAssetIdentifier(std::move(resolvedAsset.first));
        asset.setResolvedAssetPath(std::move(resolvedAsset.second));

        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::getAsset: called ayon.resolvePath() \n");
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

            std::shared_lock<std::shared_mutex> CommonCacheSharedLock(m_CommonCachesharedMutex);

            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("resolverContextCache::getAsset: insert into CommonCache \n");
            m_CommonCache.insert(asset);
        }
    }

    return asset;
};

void
resolverContextCache::removeCachedObject(const std::string &key) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::removeCachedObject (%s) \n", key.c_str());

    std::unordered_set<assetIdent>::iterator hit;

    std::unique_lock<std::shared_mutex> PreCachesharedWriteLock(m_PreCachesharedMutex);

    hit = m_PreCache.find(key);
    if (hit != m_PreCache.end()) {
        m_PreCache.erase(hit);
        PreCachesharedWriteLock.unlock();
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::removeCachedObject removed object from PreCache");
        return;
    }

    std::unique_lock<std::shared_mutex> AyonCachesharedWriteLock(m_AyonCachesharedMutex);

    hit = m_AyonCache.find(key);
    if (hit != m_AyonCache.end()) {
        m_AyonCache.erase(hit);
        AyonCachesharedWriteLock.unlock();
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::removeCachedObject removed object from AyonCache");
        return;
    }

    std::unique_lock<std::shared_mutex> CommonCachesharedWriteLock(m_CommonCachesharedMutex);
    hit = m_CommonCache.find(key);
    if (hit != m_CommonCache.end()) {
        m_CommonCache.erase(hit);
        CommonCachesharedWriteLock.unlock();
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

    std::unordered_set<assetIdent>::iterator hit;

    std::unique_lock<std::shared_mutex> PreCachesharedDellLock(m_PreCachesharedMutex);
    hit = m_PreCache.find(key);
    if (hit != m_PreCache.end()) {
        m_PreCache.erase(hit);
        PreCachesharedDellLock.unlock();
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("resolverContextCache::removeCachedObject removed object from PreCache");

        return;
    }
    else {
        switch (selectedCache) {
            case AYONCACHE:
                {
                    std::unique_lock<std::shared_mutex> AyonCachesharedDellLock(m_AyonCachesharedMutex);

                    hit = m_AyonCache.find(key);
                    if (hit != m_AyonCache.end()) {
                        m_AyonCache.erase(hit);
                        AyonCachesharedDellLock.unlock();
                        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                            .Msg("resolverContextCache::removeCachedObject removed object from AyonCache");
                        return;
                    }
                    break;
                }
            case COMMONCACHE:
                {
                    std::unique_lock<std::shared_mutex> CommonCachesharedDellLock(m_CommonCachesharedMutex);
                    hit = m_CommonCache.find(key);
                    if (hit != m_CommonCache.end()) {
                        m_CommonCache.erase(hit);
                        CommonCachesharedDellLock.unlock();
                        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                            .Msg("resolverContextCache::removeCachedObject removed object from CommonCache");
                        return;
                    }
                    break;
                }
        }
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("resolverContextCache::removeCachedObject the object could not be found");
};

void
resolverContextCache::clearCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("resolverContextCache::clearCache \n");

    std::unique_lock<std::shared_mutex> PreCachesharedMutexLock(m_PreCachesharedMutex);
    std::unique_lock<std::shared_mutex> AyonCachesharedMutexLock(m_AyonCachesharedMutex);
    std::unique_lock<std::shared_mutex> CommonCachesharedMutexLock(m_CommonCachesharedMutex);
    m_CommonCache.clear();
    m_AyonCache.clear();
    m_PreCache.clear();
};

bool
resolverContextCache::isCacheStatic() const {
    return this->m_static_cache;
};
