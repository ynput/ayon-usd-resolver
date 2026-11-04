#ifndef AR_AYONUSDRESOLVER_MEMCACHED_EXPIRY_H
#define AR_AYONUSDRESOLVER_MEMCACHED_EXPIRY_H

// Pure helpers (std only) for memcached TTL selection; unit-tested by tests/memcachedExpiryTest.cpp.

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace MemcachedExpiry {

// memcached reads a relative exptime above 30 days as an absolute unix timestamp (= already expired).
constexpr int64_t kMaxRelativeSeconds = 2592000;
// Hero/latest setting meaning "do not touch memcached for this key".
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

// Negative cache ("miss marker") for a URI AYON had no path for. Short TTL, never a path. In-process sentinel only:
// MemcachedHandler stores it as an EMPTY value, which builds without miss markers read as a plain miss (a
// non-empty marker would be returned by them as a resolved path).
// Adjacent literals on purpose: "\x01ayon" would parse as hex escape \x01a.
inline const std::string kMissMarker = "\x01" "ayon-resolver-miss";
constexpr int64_t kDefaultMissSeconds = 60;

inline bool
isMissMarker(const std::string &value) {
    return value == kMissMarker;
}

// How long a process trusts a marker it saw without reading memcached again. Markers stay out of the in-process caches
// so that a publish is seen, but one Resolve can look a URI up several times and each read is a round trip.
constexpr int64_t kKnownMissWindowSeconds = 5;

/**
 * @brief Seconds to trust a seen marker: at most kKnownMissWindowSeconds, never longer than the marker's own TTL.
 * 0 (read memcached every time) when this process has miss caching off for the URI.
 */
inline int64_t
knownMissWindowFor(int64_t missTtl) {
    return missTtl <= 0 ? 0 : std::min(missTtl, kKnownMissWindowSeconds);
}

/**
 * @brief Miss TTL env: unset -> 60, -1 = do not cache misses, >0 = seconds (clamped at 30 days).
 * 0 is refused (default + warning): a miss that never expires would hide a first publish if the watcher is down.
 */
inline Parsed
parseMissExpiry(const char *env) {
    Parsed p = parseExpiry(env, kDefaultMissSeconds, kSkipMemcached);
    if (p.warning.empty() && p.value == 0) {
        return {kDefaultMissSeconds, "0 is not allowed (a miss must expire), using default " + std::to_string(kDefaultMissSeconds)};
    }
    return p;
}

enum class VersionClass { Immutable, Hero, Latest };

/**
 * @brief Class of the URI's `version` query parameter, normalised like the server does (trimmed, lowercased, one
 * leading `v` dropped): digits -> Immutable, `hero` -> Hero, `latest`/`latestdone` -> Latest. A missing version,
 * `*` or any other value -> Latest: the server then matches any version (no stable answer), so it must never be
 * cached as immutable. With several version params, Latest > Hero > Immutable.
 */
inline VersionClass
classifyUri(const std::string &uri) {
    bool seen = false;
    VersionClass result = VersionClass::Immutable;
    const size_t q = uri.find('?');
    if (q == std::string::npos) {
        return VersionClass::Latest;
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
            if (!val.empty() && val[0] == 'v') {
                val.erase(0, 1);
            }
            seen = true;
            const bool digits = !val.empty() && std::all_of(val.begin(), val.end(), [](unsigned char c) {
                return std::isdigit(c) != 0;
            });
            if (val == "hero") {
                result = VersionClass::Hero;
            }
            else if (!digits) {
                return VersionClass::Latest;   // latest, latestdone, `*`, empty or unknown
            }
        }
        pos = amp + 1;
    }
    return seen ? result : VersionClass::Latest;
}

/**
 * @brief TTL for this URI: kSkipMemcached (-1) = bypass memcached, 0 = never expire, >0 = seconds.
 */
inline int64_t
ttlFor(const std::string &uri, int64_t generalTtl, int64_t heroTtl, int64_t latestTtl) {
    switch (classifyUri(uri)) {
        case VersionClass::Latest: return latestTtl;
        case VersionClass::Hero: return heroTtl;
        default: return generalTtl;
    }
}

/**
 * @brief TTL for a miss marker: never longer than the URI's own class TTL (a missing `latest` must not
 * outlive a found one). kSkipMemcached when either side bypasses memcached.
 */
inline int64_t
missTtlFor(int64_t classTtl, int64_t missTtl) {
    if (classTtl < 0 || missTtl < 0) {
        return kSkipMemcached;
    }
    return classTtl > 0 ? std::min(classTtl, missTtl) : missTtl;
}

}   // namespace MemcachedExpiry

#endif   // AR_AYONUSDRESOLVER_MEMCACHED_EXPIRY_H
