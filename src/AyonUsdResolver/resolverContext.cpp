#include <memory>
#include <string>

#include "codes/debugCodes.h"
#define CONVERT_STRING(string) #string
#define DEFINE_STRING(string)  CONVERT_STRING(string)

#include "resolverContext.h"

#include "pxr/pxr.h"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

std::shared_ptr<resolverContextCache> GlobalCache = std::make_shared<resolverContextCache>();

bool
getStringEndswithString(const std::string &value, const std::string &compareValue) {
    if (compareValue.size() > value.size()) {
        return false;
    }
    if (std::equal(compareValue.rbegin(), compareValue.rend(), value.rbegin())) {
        return true;
    }
    return false;
}

bool
getStringEndswithStrings(const std::string &value, const std::vector<std::string> array) {
    for (int i = 0; i < array.size(); i++) {
        if (getStringEndswithString(value, array[i])) {
            return true;
        }
    }
    return false;
}

AyonUsdResolverContext::AyonUsdResolverContext(): cache(std::shared_ptr(GlobalCache)) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::ResolverContext() - Creating new context\n");
    this->Initialize();
}

AyonUsdResolverContext::AyonUsdResolverContext(const AyonUsdResolverContext &ctx) = default;

AyonUsdResolverContext::~AyonUsdResolverContext() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::~ResolverContext() - Destructed Context\n");
}

bool
AyonUsdResolverContext::operator<(const AyonUsdResolverContext &ctx) const {
    return true;
}

bool
AyonUsdResolverContext::operator==(const AyonUsdResolverContext &ctx) const {
    return this == &ctx;
}

bool
AyonUsdResolverContext::operator!=(const AyonUsdResolverContext &ctx) const {
    return !(*this == ctx);
}

size_t
hash_value(const AyonUsdResolverContext &ctx) {
    return TfHash()(&ctx);
}
void
AyonUsdResolverContext::Initialize() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::Initialize\n");
}

void
AyonUsdResolverContext::ClearAndReinitialize() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::ClearAndReinitialize()\n");
    dropCache();
    this->Initialize();
}

void
AyonUsdResolverContext::dropCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::dropCache()\n");
    this->cache = std::make_shared<resolverContextCache>();
};

void
AyonUsdResolverContext::deleteFromCache(const std::string &key) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::deleteFromCache(%s)\n", key.c_str());
    cache->removeCachedObject(key);
};

void
AyonUsdResolverContext::clearCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::clearCache\n");
    cache->clearCache();
};

std::shared_ptr<resolverContextCache>
AyonUsdResolverContext::getCachePtr() const {
    return this->cache;
};
