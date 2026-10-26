#ifndef AR_AYONUSDRESOLVER_MEMCACHED_HANDLER_H
#define AR_AYONUSDRESOLVER_MEMCACHED_HANDLER_H

#include "../cache/assetIdentifierDef.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

/**
 * @class MemcachedHandler
 * @brief Handles memcached connection and asset resolution queries
 */
class MemcachedHandler {
    public:
        /**
         * @brief Construct a memcached handler with server list
         * @param servers Comma-separated list of memcached servers (e.g., "localhost:11211,192.168.1.100:11211")
         * @param timeoutMs Timeout for memcached operations in milliseconds
         */
        MemcachedHandler(const std::string &servers, uint32_t timeoutMs = 1000);
        ~MemcachedHandler();

        /**
         * @brief Try to get asset data from memcached
         * @param assetIdentifier The asset URI to resolve
         * @return AssetIdentifier if found in memcached, empty AssetIdentifier if not found or error
         */
        AssetIdentifier getAssetData(const std::string &assetIdentifier);

        /**
         * @brief Store asset data in memcached
         * @param assetIdentifier The asset identifier (key)
         * @param assetPath The resolved asset path (value)
         * @param expireSeconds Time to live in seconds (default 3600)
         * @return true if successful, false otherwise
         */
        bool setAssetData(const std::string &assetIdentifier, const std::string &assetPath, uint32_t expireSeconds = 3600);

        /**
         * @brief Delete asset data from memcached
         * @param assetIdentifier The asset identifier (key) to delete
         * @return true if successful or key not found, false on error
         */
        bool deleteAssetData(const std::string &assetIdentifier);

        /**
         * @brief Flush all entries from the memcached server
         * @return true if successful, false otherwise
         */
        bool flushAll();

        /**
         * @brief Check if memcached handler is properly initialized
         * @return true if connected and ready, false otherwise
         */
        bool isConnected() const;

    private:
        // Private implementation details
        class Impl;
        std::unique_ptr<Impl> pImpl;
        uint32_t m_timeoutMs;
        bool m_connected;
};

#endif  // AR_AYONUSDRESOLVER_MEMCACHED_HANDLER_H
