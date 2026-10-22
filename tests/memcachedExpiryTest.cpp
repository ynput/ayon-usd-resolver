// g++ -std=c++17 -Wall -I src/AyonUsdResolver/cache tests/memcachedExpiryTest.cpp -o /tmp/memcachedExpiryTest && /tmp/memcachedExpiryTest
#include "memcachedExpiry.h"

#include <cassert>
#include <iostream>

using namespace MemcachedExpiry;

int main() {
    const std::string base = "ayon://P//assets/x?product=usdAsset&representation=usd&version=";
    const auto L = VersionClass::Latest, H = VersionClass::Hero, I = VersionClass::Immutable;
    assert(classifyUri(base + "latest") == L);
    assert(classifyUri(base + "LATEST") == L);
    assert(classifyUri(base + " Latest ") == L);
    assert(classifyUri(base + "latestdone") == L);
    assert(classifyUri("ayon+entity://P//a?version=latest&product=x") == L);
    assert(classifyUri(base + "hero") == H);
    assert(classifyUri(base + "HERO") == H);
    assert(classifyUri(base + " Hero ") == H);
    assert(classifyUri(base + "herox") == I);
    assert(classifyUri(base + "v004") == I);
    assert(classifyUri(base + "4") == I);
    assert(classifyUri("ayon://P//a?product=x") == I);
    assert(classifyUri("ayon://P//a") == I);
    assert(classifyUri("ayon://P//a?preversion=hero") == I);
    assert(classifyUri("ayon://P//a?preversion=latest") == I);
    assert(classifyUri("ayon://P//a?version=hero&version=latest") == L);
    assert(classifyUri("ayon://P//a?version=latest&version=hero") == L);
    assert(classifyUri("ayon://P//a?product=version=latest&version=hero") == H);
    assert(classifyUri("ayon://P//a?version=hero#version=latest") == H);

    assert(ttlFor(base + "latest", 0, 5, -1) == -1);
    assert(ttlFor(base + "hero", 7, 5, -1) == 5);
    assert(ttlFor(base + "v004", 7, 5, -1) == 7);

    // hero: default 3600, min -1
    assert(parseExpiry(nullptr, 3600, -1).value == 3600 && parseExpiry(nullptr, 3600, -1).warning.empty());
    assert(parseExpiry("-1", 3600, -1).value == -1 && parseExpiry("0", 3600, -1).value == 0);
    assert(parseExpiry("60", 3600, -1).value == 60);
    assert(parseExpiry("x", 3600, -1).value == 3600 && !parseExpiry("x", 3600, -1).warning.empty());
    assert(parseExpiry("-5", 3600, -1).value == 3600);
    assert(parseExpiry("9999999", 3600, -1).value == kMaxRelativeSeconds);

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

    // miss marker + miss TTL: default 60, -1 = off, 0 refused
    assert(isMissMarker(kMissMarker) && kMissMarker[0] == '\x01' && kMissMarker.size() == 19);
    assert(!isMissMarker("") && !isMissMarker("ayon-resolver-miss") && !isMissMarker("{root[work]}/a/b.usd"));
    assert(parseMissExpiry(nullptr).value == 60 && parseMissExpiry(nullptr).warning.empty());
    assert(parseMissExpiry("-1").value == -1 && parseMissExpiry("-1").warning.empty());
    assert(parseMissExpiry("30").value == 30 && parseMissExpiry("30").warning.empty());
    assert(parseMissExpiry("0").value == 60 && !parseMissExpiry("0").warning.empty());
    assert(parseMissExpiry("abc").value == 60 && !parseMissExpiry("abc").warning.empty());
    assert(parseMissExpiry("-5").value == 60 && !parseMissExpiry("-5").warning.empty());
    assert(parseMissExpiry("9999999").value == kMaxRelativeSeconds && !parseMissExpiry("9999999").warning.empty());

    std::cout << "ok\n";
}
