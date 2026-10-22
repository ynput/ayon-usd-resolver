#ifndef AR_AYONUSDRESOLVER_MEMCACHED_EXPIRY_H
#define AR_AYONUSDRESOLVER_MEMCACHED_EXPIRY_H

// Pure helpers (std only) for memcached TTL selection; unit-tested by tests/memcachedExpiryTest.cpp.

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace MemcachedExpiry {

// memcached reads a relative exptime above 30 days as an absolute unix timestamp (= already expired).
constexpr int64_t kMaxRelativeSeconds = 2592000;
// Latest-URI setting meaning "do not touch memcached for this key".
constexpr int64_t kSkipMemcached = -1;

struct Parsed {
        int64_t value;
        std::string warning;   // non-empty when the env value was invalid or clamped
};

/**
 * @brief Parse a TTL env value. null -> defaultValue. Values below minValue or non-numeric -> defaultValue + warning.
 * Values above 30 days are clamped + warning.
 */
inline Parsed
parseExpiry(const char *env, int64_t defaultValue, int64_t minValue) {
    if (env == nullptr) {
        return {defaultValue, ""};
    }
    char *end = nullptr;
    errno = 0;
    const long long v = std::strtoll(env, &end, 10);
    if (end == env || *end != '\0' || errno != 0 || v < minValue) {
        return {defaultValue, std::string("invalid expiry '") + env + "', using default " + std::to_string(defaultValue)};
    }
    if (v > kMaxRelativeSeconds) {
        return {kMaxRelativeSeconds,
                std::string("expiry ") + env + " exceeds memcached's 30 day limit, clamped to "
                    + std::to_string(kMaxRelativeSeconds)};
    }
    return {v, ""};
}

/**
 * @brief True if the URI's `version` query parameter is `latest` or `latestdone` (trimmed, case-insensitive).
 */
inline bool
isLatestUri(const std::string &uri) {
    const size_t q = uri.find('?');
    if (q == std::string::npos) {
        return false;
    }
    size_t pos = q + 1;
    const size_t fragment = uri.find('#', pos);
    const size_t stop = fragment == std::string::npos ? uri.size() : fragment;
    while (pos <= stop) {
        size_t amp = uri.find('&', pos);
        if (amp == std::string::npos || amp > stop) {
            amp = stop;
        }
        const std::string param = uri.substr(pos, amp - pos);
        const size_t eq = param.find('=');
        if (eq != std::string::npos && param.substr(0, eq) == "version") {
            size_t b = eq + 1, e = param.size();
            while (b < e && std::isspace(static_cast<unsigned char>(param[b]))) ++b;
            while (e > b && std::isspace(static_cast<unsigned char>(param[e - 1]))) --e;
            std::string val = param.substr(b, e - b);
            for (char &c: val) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (val == "latest" || val == "latestdone") {
                return true;
            }
        }
        pos = amp + 1;
    }
    return false;
}

/**
 * @brief TTL for this URI: kSkipMemcached (-1) = bypass memcached, 0 = never expire, >0 = seconds.
 */
inline int64_t
ttlFor(const std::string &uri, int64_t generalTtl, int64_t latestTtl) {
    return isLatestUri(uri) ? latestTtl : generalTtl;
}

}   // namespace MemcachedExpiry

#endif   // AR_AYONUSDRESOLVER_MEMCACHED_EXPIRY_H
