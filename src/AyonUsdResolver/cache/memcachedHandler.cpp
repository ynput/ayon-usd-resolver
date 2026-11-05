#include "memcachedHandler.h"

#include "../codes/debugCodes.h"
#include "../config.h"
#include "memcachedChunks.h"
#include "memcachedExpiry.h"

#include "pxr/usd/ar/resolvedPath.h"

#if AYON_LIBMEMCACHED_AVAILABLE
#include "libmemcached-1.0/memcached.h"
#endif

#include "ynput/lib/logging/AyonLogger.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

/**
 * @brief Parse memcached server string into host:port pairs
 */
namespace {
/// Warn on a memcached failure. Unconditional: TF_DEBUG is opt-in, and neither memcached nor the
/// resolver exposes a client-side timeout counter, so this is the only place it can be observed.
void
logMemcachedFailure(const char* op, const std::string &key, const char* reason) {
    auto & logger = AyonLogger::getInstance();
    static const std::string kLogKeyName = "memcached";
    static const bool kRegistered = logger.registerLoggingKey(kLogKeyName);
    (void)kRegistered;
    logger.warn(logger.key(kLogKeyName), "memcached {} failed: {} (key: {})", op, reason, key);
}

/// Warn that the cache is unavailable for this process. Silent otherwise: the construction probe
/// disables the handler, so no per-lookup failure follows, and resolves still succeed via the API.
void
logMemcachedUnavailable(const std::string &servers, const char* reason) {
    auto & logger = AyonLogger::getInstance();
    static const std::string kLogKeyName = "memcached";
    static const bool kRegistered = logger.registerLoggingKey(kLogKeyName);
    (void)kRegistered;
    logger.warn(logger.key(kLogKeyName),
                "memcached unavailable: {} (servers: {}) -- cache disabled for this process, "
                "resolves go to the API",
                reason, servers);
}
}   // namespace

static std::vector<std::pair<std::string, uint16_t>> parseMemcachedServers(const std::string &serversStr) {
    std::vector<std::pair<std::string, uint16_t>> servers;
    std::istringstream iss(serversStr);
    std::string server;

    while (std::getline(iss, server, ',')) {
        // Trim whitespace
        size_t start = server.find_first_not_of(" \t");
        size_t end = server.find_last_not_of(" \t");
        if (start != std::string::npos) {
            server = server.substr(start, end - start + 1);
        }

        // Parse host:port
        size_t colonPos = server.find(':');
        if (colonPos != std::string::npos) {
            std::string host = server.substr(0, colonPos);
            std::string portStr = server.substr(colonPos + 1);
            try {
                uint16_t port = static_cast<uint16_t>(std::stoi(portStr));
                servers.emplace_back(host, port);
            }
            catch (const std::exception &e) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                    .Msg("MemcachedHandler: Invalid port in server string: %s (%s)\n", server.c_str(), e.what());
            }
        }
    }

    return servers;
}

#if AYON_LIBMEMCACHED_AVAILABLE

class MemcachedHandler::Impl {
    public:
        Impl(const std::vector<std::pair<std::string, uint16_t>> &servers, uint32_t timeoutMs)
            : m_servers(servers), m_timeoutMs(timeoutMs), m_connected(false), m_memc(nullptr) {
            if (!m_servers.empty()) {
                m_connected = tryConnect();
            }
        }

        ~Impl() {
            if (m_memc != nullptr) {
                memcached_free(m_memc);
                m_memc = nullptr;
            }
        }

        bool tryConnect() {
            if (m_servers.empty()) {
                return false;
            }

            m_memc = memcached_create(nullptr);
            if (m_memc == nullptr) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                    .Msg("MemcachedHandler: memcached_create failed\n");
                return false;
            }

            (void)memcached_behavior_set(m_memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, m_timeoutMs);
            (void)memcached_behavior_set(m_memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, m_timeoutMs);
            (void)memcached_behavior_set(m_memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 1);
            (void)memcached_behavior_set(m_memc, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, 2);
            (void)memcached_behavior_set(m_memc, MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS, 1);

            memcached_return_t rc = MEMCACHED_SUCCESS;
            memcached_server_list_st serverList = nullptr;

            for (const auto &[host, port] : m_servers) {
                serverList = memcached_server_list_append(serverList, host.c_str(), port, &rc);
                if (serverList == nullptr || memcached_failed(rc)) {
                    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                        .Msg("MemcachedHandler: Failed to append server %s:%u (%s)\n",
                             host.c_str(),
                             port,
                             memcached_strerror(m_memc, rc));
                    if (serverList != nullptr) {
                        memcached_server_list_free(serverList);
                    }
                    memcached_free(m_memc);
                    m_memc = nullptr;
                    return false;
                }
            }

            rc = memcached_server_push(m_memc, serverList);
            memcached_server_list_free(serverList);

            if (memcached_failed(rc)) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                    .Msg("MemcachedHandler: memcached_server_push failed (%s)\n", memcached_strerror(m_memc, rc));
                memcached_free(m_memc);
                m_memc = nullptr;
                return false;
            }

            // check the memcached servers health
            memcached_return_t probe = memcached_version(m_memc);
            if (memcached_failed(probe)) {
                std::string serverList;
                for (const auto &[host, port]: m_servers) {
                    if (!serverList.empty()) {
                        serverList += ",";
                    }
                    serverList += host + ":" + std::to_string(port);
                }
                logMemcachedUnavailable(serverList, memcached_strerror(m_memc, probe));
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                    .Msg("MemcachedHandler: no server answered (%s): disabling memcached client "
                         "in this process.\n",
                         memcached_strerror(m_memc, probe));
                memcached_free(m_memc);
                m_memc = nullptr;
                return false;
            }

            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("MemcachedHandler: Initialized libmemcached client with %zu servers\n", m_servers.size());
            return true;
        }

        // memcached keys are at most 250 bytes (MEMCACHED_MAX_KEY counts the NUL). A longer URI is never cached:
        // skip it quietly instead of failing (and warning) on every get/set.
        static bool keyFits(const std::string &key) {
            return key.length() < MEMCACHED_MAX_KEY;
        }

        // A reply that arrives after the client gave up stays on the connection, and the next command reads it as its
        // own: every later command is then one reply behind, for the life of the process. Closing the connection is the
        // only way to get rid of it; the next command reconnects.
        void dropConnections() {
            memcached_quit(m_memc);
        }

        // A read that used up the timeout may have left its reply behind even when libmemcached reports a plain miss.
        bool usedUpTimeout(std::chrono::steady_clock::time_point started) const {
            return std::chrono::steady_clock::now() - started >= std::chrono::milliseconds(m_timeoutMs);
        }

        std::string queryMemcached(const std::string &key) {
            if (!m_connected || m_memc == nullptr || !keyFits(key)) {
                return "";
            }

            const char* keys[] = {key.c_str()};
            const size_t lengths[] = {key.length()};
            const auto started = std::chrono::steady_clock::now();
            memcached_return_t rc = memcached_mget(m_memc, keys, lengths, 1);
            if (memcached_failed(rc)) {
                logMemcachedFailure("get", key, memcached_strerror(m_memc, rc));
                dropConnections();
                return "";
            }

            std::optional<std::string> value;
            bool foreignReply = false;
            memcached_result_st result;
            memcached_result_create(m_memc, &result);
            while (memcached_fetch_result(m_memc, &result, &rc) != nullptr) {
                // libmemcached does not check that a reply is for the key asked: never return another URI's path.
                if (key.compare(0, std::string::npos, memcached_result_key_value(&result),
                                memcached_result_key_length(&result)) != 0) {
                    foreignReply = true;
                    continue;
                }
                // An empty stored value is the miss marker.
                value = memcached_result_length(&result) == 0
                            ? std::string(MemcachedExpiry::kMissMarker)
                            : std::string(memcached_result_value(&result), memcached_result_length(&result));
            }
            memcached_result_free(&result);

            // Anything but END or NOTFOUND here is the reply to another command (STORED, for example).
            const bool wrongReply
                = foreignReply || (rc != MEMCACHED_END && rc != MEMCACHED_NOTFOUND && rc != MEMCACHED_SUCCESS);
            if (wrongReply) {
                logMemcachedFailure("get", key, foreignReply ? "reply for another key" : memcached_strerror(m_memc, rc));
            }
            else if (!value.has_value() && usedUpTimeout(started)) {
                logMemcachedFailure("get", key, "no reply within the timeout");
            }
            else {
                return value.value_or("");
            }
            dropConnections();
            return value.value_or("");
        }

        std::unordered_map<std::string, std::string> getMany(const std::vector<std::string> &keys) {
            std::unordered_map<std::string, std::string> found;
            if (!m_connected || m_memc == nullptr || keys.empty()) {
                return found;
            }
            const size_t chunks = MemcachedChunks::chunkCount(keys.size());
            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("MemcachedHandler: mget %zu keys in %zu chunks\n", keys.size(), chunks);

            std::vector<const char *> ptrs;
            std::vector<size_t> lens;
            for (size_t begin = 0; begin < keys.size(); begin += MemcachedChunks::kMaxKeysPerMget) {
                const size_t end = std::min(keys.size(), begin + MemcachedChunks::kMaxKeysPerMget);
                ptrs.clear();
                lens.clear();
                for (size_t i = begin; i < end; ++i) {
                    // One over-long key fails the whole mget; skip it (a plain miss) so the rest of the chunk still hits.
                    if (keys[i].length() >= MEMCACHED_MAX_KEY) {
                        continue;
                    }
                    ptrs.push_back(keys[i].c_str());
                    lens.push_back(keys[i].length());
                }
                if (ptrs.empty()) {
                    continue;
                }
                const auto started = std::chrono::steady_clock::now();
                memcached_return_t rc = memcached_mget(m_memc, ptrs.data(), lens.data(), ptrs.size());
                if (memcached_failed(rc)) {
                    logMemcachedFailure("mget", keys[begin] + " (+" + std::to_string(end - begin - 1) + " more)",
                                        memcached_strerror(m_memc, rc));
                    // SOME_ERRORS: one server failed, the others were sent the request. Their replies are still valid
                    // and have to be read, or their keys would go to AYON.
                    if (rc != MEMCACHED_SOME_ERRORS) {
                        dropConnections();
                        return found;
                    }
                }
                memcached_result_st result;
                memcached_result_create(m_memc, &result);
                // Drain fully: the connection is unusable for other ops until END.
                while (memcached_fetch_result(m_memc, &result, &rc) != nullptr) {
                    // Empty value = the miss marker (see queryMemcached).
                    found.emplace(std::string(memcached_result_key_value(&result), memcached_result_key_length(&result)),
                                  memcached_result_length(&result) == 0
                                      ? MemcachedExpiry::kMissMarker
                                      : std::string(memcached_result_value(&result), memcached_result_length(&result)));
                }
                memcached_result_free(&result);
                const bool wrongReply = rc != MEMCACHED_END && rc != MEMCACHED_NOTFOUND && rc != MEMCACHED_SUCCESS;
                if (wrongReply || usedUpTimeout(started)) {
                    logMemcachedFailure("mget", keys[begin] + " (+" + std::to_string(end - begin - 1) + " more)",
                                        wrongReply ? memcached_strerror(m_memc, rc) : "no complete reply within the timeout");
                    dropConnections();
                    return found;
                }
            }
            return found;
        }

        bool setMemcached(const std::string &key, const std::string &value, uint32_t expireSeconds) {
            if (!m_connected || m_memc == nullptr || !keyFits(key)) {
                return false;
            }

            // The miss marker goes on the wire as an empty value (see MemcachedExpiry::kMissMarker).
            const std::string &stored = MemcachedExpiry::isMissMarker(value) ? std::string() : value;
            memcached_return_t rc = memcached_set(m_memc,
                                                  key.c_str(),
                                                  key.length(),
                                                  stored.c_str(),
                                                  stored.length(),
                                                  static_cast<time_t>(expireSeconds),
                                                  0);
            if (memcached_failed(rc)) {
                logMemcachedFailure("set", key, memcached_strerror(m_memc, rc));
                dropConnections();
                return false;
            }

            return true;
        }

        // add, not set: never overwrites a value another process stored meanwhile.
        MissMarkerWrite addMissMarker(const std::string &key, uint32_t expireSeconds) {
            if (!m_connected || m_memc == nullptr || !keyFits(key)) {
                return MissMarkerWrite::Failed;
            }

            // The miss marker goes on the wire as an empty value (see MemcachedExpiry::kMissMarker).
            memcached_return_t rc = memcached_add(m_memc, key.c_str(), key.length(), "", 0,
                                                  static_cast<time_t>(expireSeconds), 0);
            if (rc == MEMCACHED_NOTSTORED || rc == MEMCACHED_DATA_EXISTS) {
                return MissMarkerWrite::Exists;
            }
            if (memcached_failed(rc)) {
                logMemcachedFailure("add", key, memcached_strerror(m_memc, rc));
                dropConnections();
                return MissMarkerWrite::Failed;
            }

            return MissMarkerWrite::Stored;
        }

        bool deleteMemcached(const std::string &key) {
            if (!m_connected || m_memc == nullptr) {
                return false;
            }
            if (!keyFits(key)) {
                return true;   // never stored
            }

            memcached_return_t rc = memcached_delete(m_memc, key.c_str(), key.length(), 0);
            if (memcached_failed(rc) && rc != MEMCACHED_NOTFOUND) {
                logMemcachedFailure("delete", key, memcached_strerror(m_memc, rc));
                dropConnections();
                return false;
            }

            return true;
        }

        bool isConnected() const {
            return m_connected;
        }

    private:
        std::vector<std::pair<std::string, uint16_t>> m_servers;
        uint32_t m_timeoutMs;
        bool m_connected;
        memcached_st *m_memc;
};

#else  // AYON_LIBMEMCACHED_AVAILABLE

// Stub implementation when libmemcached is not available
class MemcachedHandler::Impl {
    public:
        Impl(const std::vector<std::pair<std::string, uint16_t>> &, uint32_t) 
            : m_connected(false) {}
        ~Impl() {}
        
        bool tryConnect() { return false; }
        std::string queryMemcached(const std::string &) { return ""; }
        std::unordered_map<std::string, std::string> getMany(const std::vector<std::string> &) { return {}; }
        bool setMemcached(const std::string &, const std::string &, uint32_t) { return false; }
        MissMarkerWrite addMissMarker(const std::string &, uint32_t) { return MissMarkerWrite::Failed; }
        bool deleteMemcached(const std::string &) { return false; }
        bool isConnected() const { return m_connected; }
    
    private:
        bool m_connected;
};

#endif  // AYON_LIBMEMCACHED_AVAILABLE

// ============================================================================
// MemcachedHandler Implementation
// ============================================================================

MemcachedHandler::MemcachedHandler(const std::string &servers, uint32_t timeoutMs)
    : m_timeoutMs(timeoutMs), m_connected(false) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("MemcachedHandler::MemcachedHandler(servers=%s, timeout=%u)\n", servers.c_str(), timeoutMs);

    auto parsedServers = parseMemcachedServers(servers);
    pImpl = std::make_unique<Impl>(parsedServers, timeoutMs);
    m_connected = pImpl->isConnected();
}

MemcachedHandler::~MemcachedHandler() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("MemcachedHandler::~MemcachedHandler()\n");
}

AssetIdentifier MemcachedHandler::getAssetData(const std::string &assetIdentifier) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("MemcachedHandler::getAssetData(%s)\n", assetIdentifier.c_str());

    AssetIdentifier asset;

    if (!m_connected || !pImpl) {
        return asset;
    }

    std::string cachedPath = pImpl->queryMemcached(assetIdentifier);
    if (!cachedPath.empty()) {
        asset.setAssetIdentifier(assetIdentifier);
        asset.setResolvedAssetPath(ArResolvedPath(cachedPath));

        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("MemcachedHandler::getAssetData: Cache Hit for %s -> %s\n",
                 assetIdentifier.c_str(), cachedPath.c_str());
    }

    return asset;
}

std::unordered_map<std::string, std::string> MemcachedHandler::getMany(const std::vector<std::string> &keys) {
    if (!m_connected || !pImpl) {
        return {};
    }
    return pImpl->getMany(keys);
}

bool MemcachedHandler::setAssetData(const std::string &assetIdentifier, const std::string &assetPath,
                                     uint32_t expireSeconds) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("MemcachedHandler::setAssetData(%s, %s, %u)\n", assetIdentifier.c_str(), assetPath.c_str(),
             expireSeconds);

    if (!m_connected || !pImpl) {
        return false;
    }

    return pImpl->setMemcached(assetIdentifier, assetPath, expireSeconds);
}

MissMarkerWrite MemcachedHandler::addMissMarker(const std::string &assetIdentifier, uint32_t expireSeconds) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("MemcachedHandler::addMissMarker(%s, %u)\n", assetIdentifier.c_str(), expireSeconds);

    if (!m_connected || !pImpl) {
        return MissMarkerWrite::Failed;
    }

    return pImpl->addMissMarker(assetIdentifier, expireSeconds);
}

bool MemcachedHandler::deleteAssetData(const std::string &assetIdentifier) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("MemcachedHandler::deleteAssetData(%s)\n", assetIdentifier.c_str());

    if (!m_connected || !pImpl) {
        return false;
    }

    return pImpl->deleteMemcached(assetIdentifier);
}

bool MemcachedHandler::isConnected() const {
    return m_connected;
}
