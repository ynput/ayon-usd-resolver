#include <memory>
#include <string>

#include "codes/debugCodes.h"
#define CONVERT_STRING(string) #string
#define DEFINE_STRING(string)  CONVERT_STRING(string)

#include "resolverContext.h"
#include "Wrappers_Tokens/resolverTokens.h"

#include "pxr/pxr.h"
#include "pxr/base/tf/pathUtils.h"
#include <pxr/base/vt/value.h>
#include <pxr/base/vt/array.h>
#include "pxr/usd/sdf/layer.h"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

std::shared_ptr<ResolverContextCache> GlobalCache = std::make_shared<ResolverContextCache>();

// ResolverContextCache& GetGlobalResolverContextCache() {
//     static ResolverContextCache instance;   // lazy init on first use
//     return instance;
// }

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
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::ResolverContext() - Build timestamp: {} {}\n", __DATE__, __TIME__);
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::ResolverContext() - Creating new context\n");
    Initialize();
}

// AyonUsdResolverContext::AyonUsdResolverContext() {
//     TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::ResolverContext() - Creating new context\n");
//     cache = std::shared_ptr<ResolverContextCache>(
//         &GetGlobalResolverContextCache(),
//         [](ResolverContextCache*) {}  // no-op deleter
//     );
//     this->Initialize();
// }

AyonUsdResolverContext::AyonUsdResolverContext(const AyonUsdResolverContext &ctx) = default;

AyonUsdResolverContext::AyonUsdResolverContext(const std::string &filePath) : cache(std::shared_ptr(GlobalCache)) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::ResolverContext() - Build timestamp: {} {}\n", __DATE__, __TIME__);
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::ResolverContext() - Creating new context with defined mapping filePath\n");
    mappingFilePath = filePath;
    Initialize();
}

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
    if (!mappingFilePath.empty()) {
        _GetMappingPairsFromFile(mappingFilePath);
    }
}

void
AyonUsdResolverContext::ClearAndReinitialize() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::ClearAndReinitialize()\n");
    DropCache();
    this->Initialize();
}

void
AyonUsdResolverContext::DropCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::DropCache()\n");
    this->cache = std::make_shared<ResolverContextCache>();
};

// void AyonUsdResolverContext::DropCache() {
//     TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::DropCache()\n");
//     cache = std::shared_ptr<ResolverContextCache>(
//         &GetGlobalResolverContextCache(),
//         [](ResolverContextCache*) {}
//     );
//     cache->ClearCache();
// }

void
AyonUsdResolverContext::DeleteFromCache(const std::string &key) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::DeleteFromCache(%s)\n", key.c_str());
    cache->removeCachedObject(key);
};

void
AyonUsdResolverContext::ClearCache() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("ResolverContext::ClearCache\n");
    cache->ClearCache();
}

const std::string&
AyonUsdResolverContext::GetMappingFilePath() const {
    return mappingFilePath;
}

void
AyonUsdResolverContext::SetMappingFilePath(std::string filePath) {
    mappingFilePath = filePath;
}

void
AyonUsdResolverContext::RefreshFromMappingFilePath() {
    _getMappingPairsFromFile(mappingFilePath);
}

std::shared_ptr<ResolverContextCache>
AyonUsdResolverContext::GetCachePtr() const {
    return cache;
};

bool
AyonUsdResolverContext::_getMappingPairsFromFile(const std::string& filePath) {
    mappingPairs.clear();
    if (getStringEndswithStrings(filePath, std::vector<std::string>{".usd", ".usdc", ".usda"})) {
        return _getMappingPairsFromUsdFile(filePath);
    } else if (getStringEndswithStrings(filePath, std::vector<std::string>{".json"})) {
        return _getMappingPairsFromJsonFile(filePath);
    }

    return false;
}

bool
AyonUsdResolverContext::_getMappingPairsFromUsdFile(const std::string& filePath) {
    mappingPairs.clear();

    auto layer = SdfLayer::FindOrOpen(TfAbsPath(filePath));
    if (!layer) {
        return false;
    }
    auto layerMetaData = layer->GetCustomLayerData();
    auto mappingDataPtr = layerMetaData.GetValueAtPath(AyonUsdResolverTokens->mappingPairs);
    if (!mappingDataPtr) {
        return false;
    }
    pxr::VtStringArray mappingDataArray = mappingDataPtr->Get<pxr::VtStringArray>();
    if (mappingDataArray.size() % 2 != 0) {
        return false;
    }
    for (size_t i = 0; i < mappingDataArray.size(); i+=2) {
        this->AddMappingPair(mappingDataArray[i], mappingDataArray[i+1]);
    }
    return true;
}

bool
AyonUsdResolverContext::_getMappingPairsFromJsonFile(const std::string& filePath) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("ResolverContext::_getMappingPairsFromJsonFile(%s)\n", filePath.c_str());

    mappingPairs.clear();

    std::ifstream in(TfAbsPath(filePath));
    if (!in.is_open()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContext::_getMappingPairsFromJsonFile - Failed to open file\n");
        return false;
    }

    nlohmann::json j;
    try {
        in >> j;
    } catch (const nlohmann::json::exception& e) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContext::_getMappingPairsFromJsonFile - JSON parse error: %s\n", e.what());
        return false;
    }

    // Get the mappingPairs key
    std::string key = AyonUsdResolverTokens->mappingPairs.GetString();
    if (!j.contains(key) || !j[key].is_object()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("ResolverContext::_getMappingPairsFromJsonFile - Invalid or missing '%s'\n", key.c_str());
        return false;
    }

    for (const auto& [source, target] : j[key].items()) {
        if (target.is_string()) {
            AddMappingPair(source, target.get<std::string>());
        }
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("ResolverContext::_getMappingPairsFromJsonFile - Loaded %zu mappings\n", 
             mappingPairs.size());

    return !mappingPairs.empty();
}

void
AyonUsdResolverContext::AddMappingPair(const std::string& sourceStr, const std::string& targetStr) {
    auto map_find = mappingPairs.find(sourceStr);
    if(map_find != mappingPairs.end()) {
        map_find->second = targetStr;
    }else{
        mappingPairs.insert(std::pair<std::string, std::string>(sourceStr,targetStr));
    }
}

void
AyonUsdResolverContext::RemoveMappingByKey(const std::string& sourceStr) {
    const auto &it = mappingPairs.find(sourceStr);
    if (it != mappingPairs.end()) {
        mappingPairs.erase(it);
    }
}

void
AyonUsdResolverContext::RemoveMappingByValue(const std::string& targetStr) {
    for (auto it = mappingPairs.cbegin(); it != mappingPairs.cend();) {
        if (it->second == targetStr) {
            mappingPairs.erase(it++);
        } else {
            ++it;
        }
    }
}

void
AyonUsdResolverContext::ClearMappingPairs() {
    mappingPairs.clear();
}

const std::map<std::string, std::string>
AyonUsdResolverContext::GetMappingPairs() const {
    return std::map<std::string, std::string>(mappingPairs.begin(), mappingPairs.end());
}
