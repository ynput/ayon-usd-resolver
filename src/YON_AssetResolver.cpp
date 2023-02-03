#include "pxr/pxr.h"

#include "YON_AssetResolver.h"

#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/filesystemWritableAsset.h"
#include "pxr/usd/ar/notice.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/js/api.h"
#include "YON_AssetResolveResult.h"

#include "YON_AssetResolvingProvider_REST.h"


PXR_NAMESPACE_USING_DIRECTIVE

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(YON_AssetResolver, ArResolver);

TF_DEBUG_CODES(USD_RESOLVER_EXAMPLE);

TF_DEFINE_ENV_SETTING(USD_RESOLVER_EXAMPLE_ASSET_DIR, ".","Root of asset directory used by YON_AssetResolver.")
TF_DEFINE_PRIVATE_TOKENS(_tokens, (asset) (latest) ((version, "{$VERSION}")));

PXR_NAMESPACE_CLOSE_SCOPE

YON_AssetResolver::YON_AssetResolver(){}
YON_AssetResolver::~YON_AssetResolver() = default;

static std::string _CreateIdentifierHelper(const std::string& assetPath, const ArResolvedPath& anchorAssetPath)
{
    return "identifier";
}

std::string YON_AssetResolver::_CreateIdentifier(const std::string& assetPath, const ArResolvedPath& anchorAssetPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg("YON_AssetResolver::_CreateIdentifier('%s', '%s')\n", assetPath.c_str(), anchorAssetPath.GetPathString().c_str());

    return _CreateIdentifierHelper(assetPath, anchorAssetPath);
}

std::string YON_AssetResolver::_CreateIdentifierForNewAsset(const std::string& assetPath, const ArResolvedPath& anchorAssetPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg("YON_AssetResolver::_CreateIdentifierForNewAsset" "('%s', '%s')\n", assetPath.c_str(), anchorAssetPath.GetPathString().c_str());

    return _CreateIdentifierHelper(assetPath, anchorAssetPath);
}


ArResolvedPath YON_AssetResolver::_Resolve(const std::string& assetPath) const
{     
    auto restRequest = YON_AssetResolvingProvider_REST();
    restRequest.bEnableDebugLog = false;

    YON_AssetResolveResult Result = restRequest.Resolve(assetPath);
   
    return ArResolvedPath(Result.ResolvedPath);
}

ArResolvedPath YON_AssetResolver::_ResolveForNewAsset(const std::string& assetPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg("YON_AssetResolver::_ResolveForNewAsset('%s')\n", assetPath.c_str());

    return ArResolvedPath(assetPath);
}

std::shared_ptr<ArAsset> YON_AssetResolver::_OpenAsset(const ArResolvedPath& resolvedPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg("YON_AssetResolver::OpenAsset('%s')\n", resolvedPath.GetPathString().c_str());

    std::string filesystemPath = "";
    return ArFilesystemAsset::Open(ArResolvedPath(std::move(filesystemPath)));
}

std::shared_ptr<ArWritableAsset> YON_AssetResolver::_OpenAssetForWrite(const ArResolvedPath& resolvedPath, WriteMode writeMode) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg("YON_AssetResolver::_OpenAssetForWrite('%s', %d)\n", resolvedPath.GetPathString().c_str(), static_cast<int>(writeMode));

    std::string filesystemPath = "";

    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg("  - Opening file for write at %s\n", filesystemPath.c_str());
    return ArFilesystemWritableAsset::Create(ArResolvedPath(std::move(filesystemPath)), writeMode);
}
