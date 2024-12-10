#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "codes/debugCodes.h"

#include "redirectionFileHanlder/redirectionHanlder.h"
#define CONVERT_STRING(string) #string
#define DEFINE_STRING(string)  CONVERT_STRING(string)

#include "resolverContext.h"

#include "pxr/pxr.h"

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
    // this->Initialize();
    auto [rdf, id] = getRdFile();
    this->m_redirectionFile.emplace(rdf);
}

AyonUsdResolverContext::AyonUsdResolverContext(const std::string &configPath): cache(std::shared_ptr(GlobalCache)) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("ResolverContext::ResolverContext(const std::string &configPath) \n");

    redirectionFile* rdf = getRdFile(std::filesystem::path(configPath));
    this->m_redirectionFile.emplace(rdf);
}

AyonUsdResolverContext::AyonUsdResolverContext(const AyonUsdResolverContext &ctx) = default;

AyonUsdResolverContext::~AyonUsdResolverContext() {
    std::ostringstream oss;
    oss << static_cast<const void*>(this);
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("ResolverContext::~ResolverContext(M_ADD: '%s', M_SIZE: '%s' bytes S_CACHE: '%s', R_FILE: `%s`)\n",
             oss.str().c_str(), std::to_string(sizeof(*this)).c_str(),
             this->getCachePtr()->isCacheStatic() ? "True" : "False",
             this->getRedirectionFile() != nullptr ? "True" : "False");
}

const redirectionFile*
AyonUsdResolverContext::getRedirectionFile() const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::getRedirectionFile \n");
    if (this->m_redirectionFile.has_value()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::getRedirectionFile(found) \n");
        return this->m_redirectionFile.value();
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::getRedirectionFile(none) \n");
    return nullptr;
};

void
AyonUsdResolverContext::setRedirectionFile(const std::string &rdfPath) const {
    redirectionFile* rdf = getRdFile(std::filesystem::path(rdfPath));
    m_redirectionFile.emplace(rdf);
};

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
AyonUsdResolverContext::ClearAndReinitialize() const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::ClearAndReinitialize()\n");
    std::vector<std::string> lCache = cache->getLCache();
    cache = std::make_shared<resolverContextCache>(lCache);
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

// TODO might be better to be a Weak pointer
std::shared_ptr<resolverContextCache>
AyonUsdResolverContext::getCachePtr() const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::getCachePtr\n");
    return this->cache;
};
