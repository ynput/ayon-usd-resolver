#include <string>
#include <utility>
#include "codes/debugCodes.h"
#include "pxr/base/tf/debug.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "cache/resolverContextCache.h"
#include "helpers/resolutionFunctions.h"
#define CONVERT_STRING(string) #string
#define DEFINE_STRING(string)  CONVERT_STRING(string)

#include "resolver.h"
#include "resolverContext.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/pathUtils.h"

#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/filesystemWritableAsset.h"
#include "pxr/usd/ar/notice.h"

#include "pxr/usd/sdf/layer.h"

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(AyonUsdResolver, ArResolver);

AyonUsdResolver::AyonUsdResolver() = default;

AyonUsdResolver::~AyonUsdResolver() {
    std::ostringstream oss;
    oss << static_cast<const void*>(this);
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
        .Msg("Resolver::~AyonUsdResolver(M_ADD: '%s', M_SIZE: '%s' bytes)\n", oss.str().c_str(),
             std::to_string(sizeof(*this)).c_str());
};

std::string
AyonUsdResolver::_CreateIdentifier(const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const {
    RES_FUNCS_REMOVE_SDF_ARGS(const_cast<std::string &>(assetPath));
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
        .Msg("Resolver::_CreateIdentifier('%s', '%s')\n", assetPath.c_str(), anchorAssetPath.GetPathString().c_str());

    if (assetPath.empty()) {
        return assetPath;
    }

    if (_IsAyonPath(assetPath)) {
        return assetPath;
    }
    if (!anchorAssetPath) {
        return TfNormPath(assetPath);
    }

    const std::string anchoredAssetPath = _AnchorRelativePath(anchorAssetPath, assetPath);

    if (_IsNotFilePath(assetPath) && Resolve(anchoredAssetPath).empty()) {
        return TfNormPath(assetPath);
    }

    return TfNormPath(anchoredAssetPath);
}

std::string
AyonUsdResolver::_CreateIdentifierForNewAsset(const std::string &assetPath,
                                              const ArResolvedPath &anchorAssetPath) const {
    RES_FUNCS_REMOVE_SDF_ARGS(const_cast<std::string &>(assetPath));
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
        .Msg("Resolver::_CreateIdentifierForNewAsset('%s', '%s')\n", assetPath.c_str(),
             anchorAssetPath.GetPathString().c_str());
    if (assetPath.empty()) {
        return assetPath;
    }

    if (_IsAyonPath(assetPath)) {
        return assetPath;
    }

    if (_IsRelativePath(assetPath)) {
        return TfNormPath(anchorAssetPath ? _AnchorRelativePath(anchorAssetPath, assetPath) : TfAbsPath(assetPath));
    }

    return TfNormPath(assetPath);
}

ArResolvedPath
AyonUsdResolver::_Resolve(const std::string &assetPath) const {
    RES_FUNCS_REMOVE_SDF_ARGS(const_cast<std::string &>(assetPath));

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_Resolve( '%s' ) \n", assetPath.c_str());

    if (assetPath.empty()) {
        return ArResolvedPath();
    }

    if (SdfLayer::IsAnonymousLayerIdentifier(assetPath)) {
        return ArResolvedPath(assetPath);
    }

    const AyonUsdResolverContext* activeContext = nullptr;
    const AyonUsdResolverContext* contexts[2] = {this->_GetCurrentContextPtr(), &_fallbackContext};
    for (const AyonUsdResolverContext* ctx: contexts) {
        if (ctx) {
            activeContext = ctx;
            break;
        }
    }

    if (activeContext->getCachePtr()->isCacheStatic()) {
        ArResolvedPath cachedPath
            = activeContext->getCachePtr()->getAsset(assetPath, cacheName::AYONCACHE, true).getResolvedAssetPath();
        return cachedPath;
    }
    if (_IsAyonPath(assetPath)) {
        if (activeContext) {
            std::pair<std::string, std::string> test;
            assetIdent asset;

            std::shared_ptr<resolverContextCache> resolverCache = activeContext->getCachePtr();

            asset = resolverCache->getAsset(assetPath, cacheName::AYONCACHE, true);

            ArResolvedPath resolvedPath(asset.getResolvedAssetPath());

            if (resolvedPath) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                    .Msg("Resolver::_Resolve( '%s' ) resolved \n", resolvedPath.GetPathString().c_str());
                return resolvedPath;
            }
        }
        return ArResolvedPath();
    }

    // we also cache non ayon paths for performance this will check the non ayon path
    const AyonUsdResolverContext* pt = this->_GetCurrentContextPtr();
    if (pt) {
        ArResolvedPath path;

        path = pt->getCachePtr()->getAsset(assetPath, cacheName::COMMONCACHE, false).getResolvedAssetPath();
        if (!path.empty()) {
            return path;
        }
    }

    return ArResolvedPath(ArchAbsPath(TfNormPath(assetPath)));
}

ArResolvedPath
AyonUsdResolver::_ResolveForNewAsset(const std::string &assetPath) const {
    RES_FUNCS_REMOVE_SDF_ARGS(const_cast<std::string &>(assetPath));
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_ResolveForNewAsset('%s')\n", assetPath.c_str());
    return ArResolvedPath(assetPath.empty() ? assetPath : TfAbsPath(assetPath));
}

ArResolverContext
AyonUsdResolver::_CreateDefaultContext() const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_CreateDefaultContext()\n");
    return _fallbackContext;
}

ArResolverContext
AyonUsdResolver::_CreateDefaultContextForAsset(const std::string &assetPath) const {
    RES_FUNCS_REMOVE_SDF_ARGS(const_cast<std::string &>(assetPath));
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_CreateDefaultContextForAsset\n");

    return ArResolverContext(AyonUsdResolverContext());
}

bool
AyonUsdResolver::_IsContextDependentPath(const std::string &assetPath) const {
    RES_FUNCS_REMOVE_SDF_ARGS(const_cast<std::string &>(assetPath));
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_IsContextDependentPath()\n");
    return _IsNotFilePath(assetPath);
}

void
AyonUsdResolver::_RefreshContext(const ArResolverContext &context) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_RefreshContext()\n");
    const AyonUsdResolverContext* ctx = this->_GetCurrentContextPtr();
    if (!ctx) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_RefreshContext no ctx()\n");
        return;
    }
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_RefreshContext ctx()\n");
    ArNotice::ResolverChanged(*ctx).Send();
}

ArTimestamp
AyonUsdResolver::_GetModificationTimestamp(const std::string &assetPath, const ArResolvedPath &resolvedPath) const {
    RES_FUNCS_REMOVE_SDF_ARGS(const_cast<std::string &>(assetPath));
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
        .Msg("Resolver::GetModificationTimestamp('%s', '%s')\n", assetPath.c_str(),
             resolvedPath.GetPathString().c_str());
    return ArFilesystemAsset::GetModificationTimestamp(resolvedPath);
}

std::shared_ptr<ArAsset>
AyonUsdResolver::_OpenAsset(const ArResolvedPath &resolvedPath) const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::OpenAsset('%s')\n", resolvedPath.GetPathString().c_str());
    return ArFilesystemAsset::Open(resolvedPath);
}

std::shared_ptr<ArWritableAsset>
AyonUsdResolver::_OpenAssetForWrite(const ArResolvedPath &resolvedPath, WriteMode writeMode) const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
        .Msg("Resolver::_OpenAssetForWrite('%s', %d)\n", resolvedPath.GetPathString().c_str(),
             static_cast<int>(writeMode));
    return ArFilesystemWritableAsset::Create(resolvedPath, writeMode);
}

const AyonUsdResolverContext*
AyonUsdResolver::_GetCurrentContextPtr() const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_GetCurrentContextPtr \n");

    return _GetCurrentContextObject<AyonUsdResolverContext>();
}

const AyonUsdResolverContext*
AyonUsdResolver::GetConnectedContext() const {
    // TODO test if this implementation is valid as this->_GetCurrentContextPtr() causes a new context to be created.
    return &_fallbackContext;
};

PXR_NAMESPACE_CLOSE_SCOPE
