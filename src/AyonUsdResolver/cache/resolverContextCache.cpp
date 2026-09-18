#include "resolverContextCache.h"

#include "../codes/debugCodes.h"
#include "../config.h"
#include "../helpers/ayonApiGet.h"
#include "../helpers/resolutionFunctions.h"
#include <ynput/core/iostd/envVarHelpers.hpp>
#include <ynput/tool/ayon/rootHelpers.hpp>

#include "nlohmann/json_fwd.hpp"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/usd/ar/resolvedPath.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE

static std::mutex s_memcachedMutex;

// Absolute path -> rootless ("{root[work]}/..."), so any site can re-root it on read.
// NB rootReplaceData maps root NAME -> root PATH; destructure it the other way round and the
// match never fires, silently caching absolute paths that break every other site.
static std::string _ToRootlessPath(
    const std::string &resolvedPath,
    const std::unordered_map<std::string, std::string> &rootReplaceData) {
    std::string rootlessPath = resolvedPath;
    for (const auto &[key, root] : rootReplaceData) {
        if (!root.empty() && rootlessPath.rfind(root, 0) == 0) {
            rootlessPath = "{root[" + key + "]}" + rootlessPath.substr(root.size());
            break;
        }
    }
    return rootlessPath;
}

PinningFileHandler::PinningFileHandler(const std::string &pinningFilePath,
                                       const std::unordered_map<std::string, std::string> &rootReplaceData):
    m_pinningFilePath(pinningFilePath),
    m_rootReplaceData(rootReplaceData) {
    std::ifstream pinningFile(m_pinningFilePath);

    if (!pinningFile.is_open()) {
        throw std::runtime_error("PinningFileHandler was not able to open PinningFile: "
                                 + m_pinningFilePath.string());
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
        std::string pathed_key = ynput::tool::ayon::rootReplace(entry.key(), m_rootReplaceData);
        std::string pathed_val = ynput::tool::ayon::rootReplace(entry.value(), m_rootReplaceData);
        m_pinningFileData[pathed_key] = pathed_val;
    }
};

AssetIdentifier
PinningFileHandler::getAssetData(const std::string &resolveKey) {
    AssetIdentifier assetEntry;

    std::string pinnedAssetPath;
    try {
        pinnedAssetPath = m_pinningFileData.at(resolveKey);
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
        std::unique_ptr<AyonApi> api = getAyonApiFromEnv();
        m_ayon.emplace(std::move(api));

        m_staticCache = false;
    }
    else {
        // Static mode: load root replace data from env var for pinning file path resolution
        std::map<std::string, std::string> projectRootsEnvMap = ynput::core::iostd::getEnvMap(PINNING_ROOTS_ENV_KEY);
        m_rootReplaceData = std::unordered_map<std::string, std::string>(
            std::make_move_iterator(projectRootsEnvMap.begin()), std::make_move_iterator(projectRootsEnvMap.end()));

        m_pinningFileHandler.emplace(ynput::core::iostd::getEnvKey(PINNING_FILE_PATH_ENV_KEY),
                                           m_rootReplaceData);
    }

    // Initialize memcached handler if enabled
    const char* enable_memcached_env_var = std::getenv(ENABLE_MEMCACHED_ENV_KEY);
    const char* memcached_servers_env_var = std::getenv(MEMCACHED_SERVERS_ENV_KEY);
    
    if (enable_memcached_env_var != nullptr && std::strcmp(enable_memcached_env_var, "true") == 0 && 
        memcached_servers_env_var != nullptr) {
        uint32_t timeout_ms = 1000;  // Default 1 second timeout
        const char* timeout_env_var = std::getenv(MEMCACHED_TIMEOUT_ENV_KEY);
        if (timeout_env_var != nullptr) {
            try {
                timeout_ms = std::stoul(timeout_env_var);
            }
            catch (const std::exception &e) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                    .Msg("ResolverContextCache: Invalid timeout value, using default: %s\n", e.what());
            }
        }
        
        try {
            auto memcached = std::make_unique<MemcachedHandler>(memcached_servers_env_var, timeout_ms);
            if (memcached->isConnected()) {
                m_memcached.emplace(std::move(memcached));
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                    .Msg("ResolverContextCache: Memcached handler initialized successfully\n");

                // Fetch site roots from server once to use for memcached path root replacement,
                // avoiding the need to set AYON_USD_RESOLVER_PINNING_ROOTS manually.
                if (m_ayon.has_value()) {
                    m_rootReplaceData = m_ayon->get()->getSiteRoots();
                    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                        .Msg("ResolverContextCache: Loaded site roots from server for memcached path processing\n");
                }
            }
            else {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                    .Msg("ResolverContextCache: Failed to connect to memcached servers\n");
            }
        }
        catch (const std::exception &e) {
            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("ResolverContextCache: Exception initializing memcached: %s\n", e.what());
        }
    }
};

ResolverContextCache::~ResolverContextCache() {
};

void
ResolverContextCache::printCache() const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::printCache \n");

    std::shared_lock<std::shared_mutex> PreCacheReadLock(m_PreCacheSharedMutex);
    std::shared_lock<std::shared_mutex> AyonCacheReadLock(m_AyonCacheSharedMutex);
    std::shared_lock<std::shared_mutex> CommonCacheReadLock(m_CommonCacheSharedMutex);
    std::cout << "Printing out the Cache Entries \n";

    std::cout << "PreCache size: " << m_PreCache.size() << "\n";
    for (const auto &assetIdentifierInstance: m_PreCache) {
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

    std::unique_lock<std::shared_mutex> PreCacheWriteLock(m_PreCacheSharedMutex);
    std::unique_lock<std::shared_mutex> AyonCacheWriteLock(m_AyonCacheSharedMutex);

    m_PreCache.insert(std::move(sourceAssetIdent));
};

void
ResolverContextCache::migratePreCacheIntoAyonCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::migratePreCacheIntoAyonCache \n");
    std::unique_lock<std::shared_mutex> PreCacheWriteLock(m_PreCacheSharedMutex);
    std::unique_lock<std::shared_mutex> AyonCacheWriteLock(m_AyonCacheSharedMutex);

    m_AyonCache.reserve(m_AyonCache.size() + m_PreCache.size());
    m_AyonCache.insert(std::make_move_iterator(m_PreCache.begin()), std::make_move_iterator(m_PreCache.end()));
    m_PreCache.clear();
};

/**
 * @brief Migrate all entries from the PreCache into the AyonCache.
 * 
 * This function is called when the PreCache reaches its maximum size.
 * It moves all entries from the PreCache into the AyonCache and clears the PreCache.
 */

std::optional<std::string>
ResolverContextCache::inProcessResolved(const std::string &uriPath) const {
    const AssetIdentifier key(uriPath);
    {
        std::shared_lock<std::shared_mutex> lock(m_PreCacheSharedMutex);
        auto hit = m_PreCache.find(key);
        if (hit != m_PreCache.end()) {
            return hit->getResolvedAssetPath().GetPathString();
        }
    }
    {
        std::shared_lock<std::shared_mutex> lock(m_AyonCacheSharedMutex);
        auto hit = m_AyonCache.find(key);
        if (hit != m_AyonCache.end()) {
            return hit->getResolvedAssetPath().GetPathString();
        }
    }
    {
        std::shared_lock<std::shared_mutex> lock(m_CommonCacheSharedMutex);
        auto hit = m_CommonCache.find(key);
        if (hit != m_CommonCache.end()) {
            return hit->getResolvedAssetPath().GetPathString();
        }
    }
    return std::nullopt;
};

/**
 * @brief Batch warm the Resolver Context Cache with a list of URI paths.
 * 
 * @param uriPaths A vector of URI paths to be resolved and cached.
 * @return std::unordered_map<std::string, std::string> A map of resolved URI paths.
 */

std::unordered_map<std::string, std::string>
ResolverContextCache::batchWarm(std::vector<std::string> &uriPaths) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("ResolverContextCache::batchWarm: %zu uris \n", uriPaths.size());

    std::unordered_map<std::string, std::string> resolved;
    if (m_staticCache || uriPaths.empty()) {
        return resolved;
    }

    // Memcached first: any URI already resolved by another session, machine or DCC costs a
    // local-network lookup instead of a slot in the batched server request. Without this the
    // prewarm pass would re-resolve the whole frontier server-side on every machine, and the
    // per-asset _Resolve() calls afterwards would all hit PreCache — so the shared cache
    // would never be read.
    // Drop URIs an in-process cache can already answer. memcached gets are one network round
    // trip EACH (serial — libmemcached, no mget), so re-querying what PreCache already holds is
    // pure cost, and it compounds per stage: a 24-stage session issued ~3623 gets where 470
    // suffice. Their paths still go into `resolved` so the prewarm BFS can descend through them.
    std::vector<std::string> pending;
    pending.reserve(uriPaths.size());
    for (const auto &uriPath: uriPaths) {
        if (std::optional<std::string> cached = inProcessResolved(uriPath); cached.has_value()) {
            resolved.emplace(uriPath, *cached);
            continue;
        }
        pending.push_back(uriPath);
    }
    if (pending.empty()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::batchWarm: all %zu uris already cached in-process \n", uriPaths.size());
        return resolved;
    }

    std::vector<std::string> misses;
    if (m_memcached.has_value() && m_memcached->get()->isConnected()) {
        misses.reserve(pending.size());
        for (const auto &uriPath: pending) {
            // Lock per call, not around the loop: s_memcachedMutex is process-wide, so holding
            // it for a whole frontier would stall every other resolver thread's memcached path
            // for the length of the prewarm.
            AssetIdentifier asset;
            {
                std::lock_guard<std::mutex> lock(s_memcachedMutex);
                asset = m_memcached->get()->getAssetData(uriPath);
            }
            if (asset.isEmpty()) {
                misses.push_back(uriPath);
                continue;
            }
            // Cached values are rootless; apply this site's roots before use.
            std::string resolvedPath =
                ynput::tool::ayon::rootReplace(asset.getResolvedAssetPath().GetPathString(), m_rootReplaceData);
            asset.setResolvedAssetPath(ArResolvedPath(resolvedPath));
            this->insert(asset);
            resolved.emplace(asset.getAssetIdentifier(), resolvedPath);
        }
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::batchWarm: memcached %zu hits, %zu misses \n", resolved.size(), misses.size());
    }
    else {
        misses = pending;
    }

    if (misses.empty()) {
        return resolved;
    }

    // One batched request over the persistent keep-alive client: few round-trips,
    // deterministic, connection reused.
    std::unordered_map<std::string, std::string> fetched = m_ayon->get()->batchResolvePathSerial(misses);

    for (const auto &entry: fetched) {
        if (entry.first.empty() || entry.second.empty()) {
            continue;
        }
        AssetIdentifier asset;
        asset.setAssetIdentifier(entry.first);
        asset.setResolvedAssetPath(entry.second);
        this->insert(asset);
        resolved.emplace(entry.first, entry.second);
    }

    // Write the freshly resolved entries through, so the next session/machine on this
    // memcached instance prewarms without touching the AYON server at all.
    if (m_memcached.has_value() && m_memcached->get()->isConnected()) {
        for (const auto &entry: fetched) {
            if (entry.first.empty() || entry.second.empty()) {
                continue;
            }
            const std::string rootlessPath = _ToRootlessPath(entry.second, m_rootReplaceData);
            std::lock_guard<std::mutex> lock(s_memcachedMutex);
            m_memcached->get()->setAssetData(entry.first, rootlessPath);
        }
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::batchWarm: stored %zu rootless results in memcached \n", fetched.size());
    }

    return resolved;
};

/**
 * @brief Retrieve an asset from the cache.
 * 
 * This function attempts to retrieve the asset identified by the given assetIdentifier
 * from the specified cache (PreCache, AyonCache, or CommonCache). If the asset is found,
 * it is returned; otherwise, an empty AssetIdentifier is returned.
 * 
 * @param assetIdentifier The identifier of the asset to retrieve.
 * @param selectedCache The cache to search for the asset.
 * @param isAyonPath Indicates whether the asset path is an Ayon path.
 * @return The retrieved AssetIdentifier, or an empty AssetIdentifier if not found.
 */

AssetIdentifier
ResolverContextCache::getAsset(const std::string &assetIdentifier,
                               const CacheName selectedCache,
                               const bool isAyonPath) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::getAsset: (%s) - Using cache %s \n", assetIdentifier.c_str(), (selectedCache == CacheName::AYONCACHE ? "AyonCache" : "CommonCache"));

    AssetIdentifier asset;

    if (assetIdentifier.empty()) {
        return asset;
    }
    if (m_staticCache) {
        return m_pinningFileHandler->getAssetData(assetIdentifier);
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
    if (!asset.isEmpty()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::getAsset: Cache Hit with (%s) with (%s) \n", asset.getAssetIdentifier().c_str(),
                 asset.getResolvedAssetPath().GetPathString().c_str());
        return asset;
    }

    // Try memcached as second-level cache before calling REST API
    if (m_memcached.has_value() && m_memcached->get()->isConnected()) {
        std::lock_guard<std::mutex> lock(s_memcachedMutex);
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::getAsset: Checking memcached\n");
        asset = m_memcached->get()->getAssetData(assetIdentifier);
        if (!asset.isEmpty()) {
            // Apply root replacement to convert rootless path to absolute path
            std::string resolvedPath = ynput::tool::ayon::rootReplace(
                asset.getResolvedAssetPath().GetPathString(), m_rootReplaceData);
            asset.setResolvedAssetPath(ArResolvedPath(resolvedPath));

            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("ResolverContextCache::getAsset: Memcached Hit with (%s) with (%s) \n",
                     asset.getAssetIdentifier().c_str(), asset.getResolvedAssetPath().GetPathString().c_str());
            // Cache result locally for faster future lookups
            this->insert(asset);
            return asset;
        }
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::getAsset: No Cache Hit \n");
    if (isAyonPath) {
        std::pair<std::string, std::string> resolvedAsset = m_ayon->get()->resolvePath(assetIdentifier);

        asset.setAssetIdentifier(std::move(resolvedAsset.first));
        asset.setResolvedAssetPath(std::move(resolvedAsset.second));

        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::getAsset: called ayon.resolvePath() \n");
        this->insert(asset);

        // Also store in memcached for distributed caching.
        // Store the rootless path (e.g. {root[work]}/...) so that other platforms can apply
        // their own root via rootReplace on retrieval.
        if (m_memcached.has_value() && m_memcached->get()->isConnected() && !asset.isEmpty()) {
            const std::string rootlessPath
                = _ToRootlessPath(asset.getResolvedAssetPath().GetPathString(), m_rootReplaceData);
            std::lock_guard<std::mutex> lock(s_memcachedMutex);
            m_memcached->get()->setAssetData(asset.getAssetIdentifier(), rootlessPath);
            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("ResolverContextCache::getAsset: Stored rootless result in memcached\n");
        }
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

/**
 * @brief Remove a cached object from all caches and memcached.
 * 
 * This function attempts to remove the object identified by the given key from the PreCache,
 * AyonCache, and CommonCache. If the object is found and removed, it also deletes the corresponding
 * entry from memcached if it is connected.
 * 
 * @param key The key identifying the cached object to be removed.
 */

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
        if (m_memcached.has_value() && m_memcached->get()->isConnected()) {
            std::lock_guard<std::mutex> lock(s_memcachedMutex);
            m_memcached->get()->deleteAssetData(key);
        }
        return;
    }

    std::unique_lock<std::shared_mutex> AyonCachesharedWriteLock(m_AyonCacheSharedMutex);

    hit = m_AyonCache.find(key);
    if (hit != m_AyonCache.end()) {
        m_AyonCache.erase(hit);
        AyonCachesharedWriteLock.unlock();
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::removeCachedObject removed object from AyonCache");
        if (m_memcached.has_value() && m_memcached->get()->isConnected()) {
            std::lock_guard<std::mutex> lock(s_memcachedMutex);
            m_memcached->get()->deleteAssetData(key);
        }
        return;
    }

    std::unique_lock<std::shared_mutex> CommonCachesharedWriteLock(m_CommonCacheSharedMutex);
    hit = m_CommonCache.find(key);
    if (hit != m_CommonCache.end()) {
        m_CommonCache.erase(hit);
        CommonCachesharedWriteLock.unlock();
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContextCache::removeCachedObject removed object from CommonCache");
        if (m_memcached.has_value() && m_memcached->get()->isConnected()) {
            std::lock_guard<std::mutex> lock(s_memcachedMutex);
            m_memcached->get()->deleteAssetData(key);
        }
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
        if (m_memcached.has_value() && m_memcached->get()->isConnected()) {
            std::lock_guard<std::mutex> lock(s_memcachedMutex);
            m_memcached->get()->deleteAssetData(key);
        }
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
                        if (m_memcached.has_value() && m_memcached->get()->isConnected()) {
                            std::lock_guard<std::mutex> lock(s_memcachedMutex);
                            m_memcached->get()->deleteAssetData(key);
                        }
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
                        if (m_memcached.has_value() && m_memcached->get()->isConnected()) {
                            std::lock_guard<std::mutex> lock(s_memcachedMutex);
                            m_memcached->get()->deleteAssetData(key);
                        }
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
ResolverContextCache::ClearCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::ClearCache \n");

    std::unique_lock<std::shared_mutex> PreCachesharedMutexLock(m_PreCacheSharedMutex);
    std::unique_lock<std::shared_mutex> AyonCachesharedMutexLock(m_AyonCacheSharedMutex);
    std::unique_lock<std::shared_mutex> CommonCachesharedMutexLock(m_CommonCacheSharedMutex);
    m_CommonCache.clear();
    m_AyonCache.clear();
    m_PreCache.clear();

    if (m_memcached.has_value() && m_memcached->get()->isConnected()) {
        std::lock_guard<std::mutex> lock(s_memcachedMutex);
        m_memcached->get()->flushAll();
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContextCache::ClearCache flushed memcached\n");
    }
};

bool
ResolverContextCache::isCacheStatic() const {
    return m_staticCache;
};
