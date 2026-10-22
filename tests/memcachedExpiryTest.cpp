// g++ -std=c++17 -Wall -I src/AyonUsdResolver/cache tests/memcachedExpiryTest.cpp -o /tmp/memcachedExpiryTest && /tmp/memcachedExpiryTest
#include "memcachedExpiry.h"

#include <cassert>
#include <iostream>

using namespace MemcachedExpiry;

int main() {
    const std::string base = "ayon://P//assets/x?product=usdAsset&representation=usd&version=";
    assert(isLatestUri(base + "latest"));
    assert(isLatestUri(base + "LATEST"));
    assert(isLatestUri(base + " Latest "));
    assert(isLatestUri(base + "latestdone"));
    assert(isLatestUri("ayon+entity://P//a?version=latest&product=x"));
    assert(!isLatestUri(base + "hero"));
    assert(!isLatestUri(base + "v004"));
    assert(!isLatestUri("ayon://P//a?product=x"));
    assert(!isLatestUri("ayon://P//a"));
    assert(!isLatestUri("ayon://P//a?preversion=latest"));
    assert(!isLatestUri("ayon://P//a?product=version=latest&version=hero"));
    assert(!isLatestUri("ayon://P//a?version=hero#version=latest"));

    assert(ttlFor(base + "latest", 0, -1) == -1);
    assert(ttlFor(base + "hero", 7, -1) == 7);

    // general: default 0, min 0
    assert(parseExpiry(nullptr, 0, 0).value == 0);
    assert(parseExpiry("0", 0, 0).value == 0);
    assert(parseExpiry("3600", 0, 0).value == 3600);
    assert(parseExpiry("9999999", 0, 0).value == kMaxRelativeSeconds && !parseExpiry("9999999", 0, 0).warning.empty());
    assert(parseExpiry("abc", 0, 0).value == 0 && !parseExpiry("abc", 0, 0).warning.empty());
    assert(parseExpiry("-1", 0, 0).value == 0 && !parseExpiry("-1", 0, 0).warning.empty());
    // latest: default -1, min -1
    assert(parseExpiry(nullptr, -1, -1).value == -1 && parseExpiry(nullptr, -1, -1).warning.empty());
    assert(parseExpiry("-1", -1, -1).value == -1 && parseExpiry("-1", -1, -1).warning.empty());
    assert(parseExpiry("0", -1, -1).value == 0);
    assert(parseExpiry("3600", -1, -1).value == 3600);
    assert(parseExpiry("-5", -1, -1).value == -1 && !parseExpiry("-5", -1, -1).warning.empty());
    assert(parseExpiry("12x", -1, -1).value == -1);
    assert(parseExpiry("", -1, -1).value == -1);

    std::cout << "ok\n";
}
