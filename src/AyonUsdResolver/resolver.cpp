#include <csignal>
#include <filesystem>
#include <iostream>
#include <optional>
#include <ostream>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include "pxr/base/tf/token.h"
#include "redirectionFileHanlder/redirectionHanlder.h"

#define CONVERT_STRING(string) #string
#define DEFINE_STRING(string)  CONVERT_STRING(string)

#include "resolver.h"
#include "resolverContext.h"
#include "codes/debugCodes.h"
#include "cache/resolverContextCache.h"
#include "helpers/resolutionFunctions.h"

#include "pxr/pxr.h"

#include "pxr/base/arch/systemInfo.h"

#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/debug.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/writableAsset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/filesystemWritableAsset.h"
#include "pxr/usd/ar/notice.h"

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
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
        .Msg("Resolver::_CreateIdentifier('%s', '%s')\n", assetPath.c_str(), anchorAssetPath.GetPathString().c_str());

    if (assetPath.empty()) {
        return assetPath;
    }

    if (_IsAyonPath(assetPath)) {
        return _Resolve(assetPath);
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
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
        .Msg("Resolver::_CreateIdentifierForNewAsset('%s', '%s')\n", assetPath.c_str(),
             anchorAssetPath.GetPathString().c_str());
    if (assetPath.empty()) {
        return assetPath;
    }

    if (_IsAyonPath(assetPath)) {
        return _Resolve(assetPath);
    }

    if (_IsRelativePath(assetPath)) {
        return TfNormPath(anchorAssetPath ? _AnchorRelativePath(anchorAssetPath, assetPath) : TfAbsPath(assetPath));
    }

    return TfNormPath(assetPath);
}

ArResolvedPath
AyonUsdResolver::_Resolve(const std::string &assetPath) const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_Resolve( '%s' ) \n", assetPath.c_str());

    if (assetPath.empty()) {
        return ArResolvedPath();
    }

    const AyonUsdResolverContext* context = this->_GetCurrentContextPtr();
    if (!context) {
        context = &_fallbackContext;
    }

    const redirectionFile* rdf = context->getRedirectionFile();
    if (rdf != nullptr) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
            .Msg("Resolver::_Resolve RDF found %s \n", rdf->getLayers().at(0).string().c_str());
        std::optional<std::pair<std::string, std::string>> rdfAsset = rdf->getRedirectionForKey(assetPath);
        if (rdfAsset.has_value()) {
            TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                .Msg("Resolver::_Resolve redirectoin found found for %s :: %s\n", assetPath.c_str(),
                     rdfAsset->second.c_str());
            return ArResolvedPath(rdfAsset->second);
        }
    }

    if (_IsAyonPath(assetPath)) {
        assetIdent asset;

        std::shared_ptr<resolverContextCache> resolverCache = context->getCachePtr();

        std::string cleanAssetPath = assetPath;
        RES_FUNCS_REMOVE_SDF_ARGS(cleanAssetPath);

        asset = resolverCache->getAsset(cleanAssetPath, cacheName::AYONCACHE, true);

        size_t pos = assetPath.find(cleanAssetPath);
        std::string sdfArgs;
        if (pos != std::string::npos) {
            sdfArgs = assetPath.substr(pos + cleanAssetPath.length());
        }

        ArResolvedPath resolvedPath(sdfArgs.empty() ? asset.getResolvedAssetPath() :
                                                      asset.getResolvedAssetPath().GetPathString() + sdfArgs);

        if (resolvedPath) {
            TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                .Msg("Resolver::_Resolve( '%s' ) resolved \n", resolvedPath.GetPathString().c_str());
            return resolvedPath;
        }
        return ArResolvedPath();
    }

    if (_IsRelativePath(assetPath)) {
        ArResolvedPath resolvedPath = _ResolveAnchored(ArchGetCwd(), assetPath);
        if (resolvedPath) {
            return resolvedPath;
        }

        return ArResolvedPath();
    }

    return _ResolveAnchored(std::string(), assetPath);
}

ArResolvedPath
AyonUsdResolver::_ResolveForNewAsset(const std::string &assetPath) const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_ResolveForNewAsset('%s')\n", assetPath.c_str());
    return ArResolvedPath(assetPath.empty() ? assetPath : TfAbsPath(assetPath));
}

ArResolverContext
AyonUsdResolver::_CreateDefaultContext() const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_CreateDefaultContext()\n");
    return _fallbackContext;
}

// TODO we can check if a resolver allready has the given file connected.
ArResolverContext
AyonUsdResolver::_CreateDefaultContextForAsset(const std::string &assetPath) const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_CreateDefaultContextForAsset(%s)\n", assetPath.c_str());

    if (!assetPath.empty()) {
        pxr::SdfLayerRefPtr topLayerFile = pxr::SdfLayer::FindOrOpen(assetPath);
        if (topLayerFile->GetCustomLayerData().size() < 1) {
            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("Resolver::_CreateDefaultContextForAsset() no redirectionFile provided by file %s\n",
                     assetPath.c_str());
            return ArResolverContext(AyonUsdResolverContext());
        }
        std::string rdfPath = topLayerFile->GetCustomLayerData()
                                  .GetValueAtPath("AyonUsdResolverRedirectionFile")
                                  ->Get<pxr::TfToken>()
                                  .GetString();
        if (_IsAyonPath(rdfPath)) {
            AyonApi con = AyonApi();
            // TODO this needs to be guarded against none return
            rdfPath = con.resolvePath(rdfPath).second;
        }
        else if (!std::filesystem::path(rdfPath).is_absolute()) {
            rdfPath = std::filesystem::canonical(std::filesystem::path(assetPath).parent_path()
                                                 / std::filesystem::path(rdfPath))
                          .string();
        }
        return ArResolverContext(AyonUsdResolverContext(rdfPath));
    }
    return ArResolverContext(AyonUsdResolverContext());
}

bool
AyonUsdResolver::_IsContextDependentPath(const std::string &assetPath) const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_IsContextDependentPath(%s)\n", assetPath.c_str());
    if (_IsAyonPath(assetPath)) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_IsContextDependentPath(true)\n");
        return true;
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_IsContextDependentPath(false)\n");
    return _IsNotFilePath(assetPath);
}

void
AyonUsdResolver::_RefreshContext(const ArResolverContext &context) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_RefreshContext()\n");

    const AyonUsdResolverContext* ctx = context.Get<AyonUsdResolverContext>();
    if (!ctx) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_RefreshContext no ctx()\n");
        return;
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::_RefreshContext ctx()\n");
    ctx->ClearAndReinitialize();

    // TODO we need to clear the redirection file
    ArNotice::ResolverChanged(*ctx).Send();
}

ArTimestamp
AyonUsdResolver::_GetModificationTimestamp(const std::string &assetPath, const ArResolvedPath &resolvedPath) const {
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
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::GetConnectedContext \n");

    const AyonUsdResolverContext* context = this->_GetCurrentContextPtr();
    if (!context) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::GetConnectedContext _fallbackContext\n");
        context = &_fallbackContext;
    }
    return context;
};

const redirectionFile*
AyonUsdResolver::getRedirectionFile() const {
    const AyonUsdResolverContext* context = this->_GetCurrentContextPtr();
    if (!context) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::GetConnectedContext _fallbackContext\n");
        context = &_fallbackContext;
    }
    return context->getRedirectionFile();
};

void
AyonUsdResolver::setRedirectionFileForCtx(const std::string &rdfFilePath) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::setRedirectionFileForCtx\n");
    const AyonUsdResolverContext* ctx = this->_GetCurrentContextPtr();
    if (!ctx) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT).Msg("Resolver::setRedirectionFileForCtx _fallbackContext\n");
        ctx = &_fallbackContext;
    }

    const redirectionFile* rdf = ctx->getRedirectionFile();
    if (rdf != nullptr) {
        // TODO this should not error but report to the caller.
        throw std::runtime_error("Cant add a rdf file to a context that allready has an rdf");
    }
    ctx->setRedirectionFile(rdfFilePath);

    ArNotice::ResolverChanged(*ctx).Send();
};
PXR_NAMESPACE_CLOSE_SCOPE
