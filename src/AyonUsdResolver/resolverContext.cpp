#include <string>
#include <utility>
#define CONVERT_STRING(string) #string
#define DEFINE_STRING(string)  CONVERT_STRING(string)

#include "resolver.h"
#include "resolverContext.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

static std::mutex g_resolver_query_mutex;

PXR_NAMESPACE_USING_DIRECTIVE

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

size_t
hash_value(const AyonUsdResolverContext &ctx) {
    return TfHash()(ctx);
}

//------------------------------ IMPL Context ------------------------------

AyonUsdResolverContext::AyonUsdResolverContext() {
    this->Initialize();
}

AyonUsdResolverContext::AyonUsdResolverContext(const AyonUsdResolverContext &ctx) = default;

// convinience init function that gets called by the construcktors
void
AyonUsdResolverContext::Initialize() {
}

void
AyonUsdResolverContext::ClearAndReinitialize() {
    // Clear all the things

    this->Initialize();
}

bool
AyonUsdResolverContext::operator<(const AyonUsdResolverContext &ctx) const {
    // This is a no-op for now, as it doesn't get used for now.
    return true;
}

bool
AyonUsdResolverContext::operator==(const AyonUsdResolverContext &ctx) const {
    return this == &ctx;
}

bool
AyonUsdResolverContext::operator!=(const AyonUsdResolverContext &ctx) const {
    return this != &ctx;
}
