#include "memcachedHandler.h"

#include "../codes/debugCodes.h"
#include "../config.h"

#include "pxr/usd/ar/resolvedPath.h"

#if AYON_LIBMEMCACHED_AVAILABLE
#include "libmemcached-1.0/memcached.h"
#endif

#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

/**
 * @brief Parse memcached server string into host:port pairs
 */
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

            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("MemcachedHandler: Initialized libmemcached client with %zu servers\n", m_servers.size());
            return true;
        }

        std::string queryMemcached(const std::string &key) {
            if (!m_connected || m_memc == nullptr) {
                return "";
            }

            size_t valueLength = 0;
            uint32_t flags = 0;
            memcached_return_t rc = MEMCACHED_SUCCESS;
            char *value = memcached_get(m_memc, key.c_str(), key.length(), &valueLength, &flags, &rc);

            if (rc == MEMCACHED_NOTFOUND) {
                return "";
            }

            if (memcached_failed(rc) || value == nullptr) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                    .Msg("MemcachedHandler: get failed for key %s (%s)\n",
                         key.c_str(),
                         memcached_strerror(m_memc, rc));
                return "";
            }

            std::string result(value, valueLength);
            free(value);
            return result;
        }

        bool setMemcached(const std::string &key, const std::string &value, uint32_t expireSeconds) {
            if (!m_connected || m_memc == nullptr) {
                return false;
            }

            memcached_return_t rc = memcached_set(m_memc,
                                                  key.c_str(),
                                                  key.length(),
                                                  value.c_str(),
                                                  value.length(),
                                                  static_cast<time_t>(expireSeconds),
                                                  0);
            if (memcached_failed(rc)) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                    .Msg("MemcachedHandler: set failed for key %s (%s)\n",
                         key.c_str(),
                         memcached_strerror(m_memc, rc));
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
        bool setMemcached(const std::string &, const std::string &, uint32_t) { return false; }
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

bool MemcachedHandler::isConnected() const {
    return m_connected;
}
