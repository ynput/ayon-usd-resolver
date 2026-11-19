#ifndef AR_AYONUSDRESOLVER_MEMCACHED_CHUNKS_H
#define AR_AYONUSDRESOLVER_MEMCACHED_CHUNKS_H

// Pure helper (std only) for splitting an mget key list; unit-tested by tests/memcachedChunksTest.cpp.

#include <cstddef>

namespace MemcachedChunks {

/// Max keys per memcached_mget request.
constexpr size_t kMaxKeysPerMget = 256;

/// Number of mget chunks needed for n keys.
constexpr size_t
chunkCount(size_t n, size_t chunk = kMaxKeysPerMget) {
    return chunk == 0 ? 0 : (n + chunk - 1) / chunk;
}

}   // namespace MemcachedChunks

#endif
