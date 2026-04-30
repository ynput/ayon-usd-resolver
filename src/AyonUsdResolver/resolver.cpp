#include "resolver.h"
#include "resolverContext.h"
#include "codes/debugCodes.h"
#include "cache/resolverContextCache.h"
#include "helpers/resolutionFunctions.h"

#include <pxr/pxr.h>
#include <pxr/base/arch/systemInfo.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/debug.h>
#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/resolverContext.h>
#include <pxr/usd/ar/writableAsset.h>
#include <pxr/usd/ar/resolvedPath.h>
#include <pxr/usd/ar/defineResolver.h>
#include <pxr/usd/ar/filesystemAsset.h>
#include <pxr/usd/ar/filesystemWritableAsset.h>
#include <pxr/usd/ar/notice.h>

#include <sstream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(AyonUsdResolver, ArResolver);

AyonUsdResolver::AyonUsdResolver() = default;

AyonUsdResolver::~AyonUsdResolver() {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
        .Msg("Resolver::~AyonUsdResolver() (size: %zu bytes)\n", sizeof(*this));
}

std::string
AyonUsdResolver::_CreateIdentifier(const std::string &assetPath, const ArResolvedPath &anchorAssetPath) const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
        .Msg("Resolver::_CreateIdentifier('%s', '%s')\n", assetPath.c_str(), anchorAssetPath.GetPathString().c_str());

    if (assetPath.empty()) {
        return assetPath;
    }

    if (_IsAyonPath(assetPath)) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
            .Msg("Resolver::_CreateIdentifier - AYON URI, returning as-is\n");
        return assetPath;  // Keep AYON URI for later context-dependent resolution
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
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
            .Msg("Resolver::_CreateIdentifierForNewAsset - AYON URI, returning as-is\n");
        return assetPath;
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

    const std::string* pathToResolve = &assetPath;
    std::string mappedPath;
    const AyonUsdResolverContext* currentCtx = this->_GetCurrentContextPtr();

    // KEY FIX: If bound context has mappings, sync them to fallback immediately
    // This ensures the VERY NEXT resolve with nil context also sees the mappings
    if (currentCtx && !currentCtx->GetMappingPairs().empty()) {
        if (currentCtx->GetMappingPairs() != _fallbackContext.GetMappingPairs()) {
            TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                .Msg("Resolver::_Resolve - Syncing bound context (%zu mappings) to fallback\n",
                     currentCtx->GetMappingPairs().size());
            const_cast<AyonUsdResolver*>(this)->_fallbackContext = *currentCtx;
        }
    }

    const AyonUsdResolverContext* contexts[2] = {currentCtx, &_fallbackContext};
    
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
        .Msg("Resolver::_Resolve - Current context: %p, Fallback context: %p\n", 
            contexts[0], contexts[1]);
    
    for (const AyonUsdResolverContext* ctx: contexts) {
        if (ctx) {
            const auto &mappingPairs = ctx->GetMappingPairs();
            
            TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                .Msg("Resolver::_Resolve - Checking context with %zu mappings\n", 
                    mappingPairs.size());
            
            auto map_find = mappingPairs.find(assetPath);
            
            if (map_find != mappingPairs.end()) {
                mappedPath = map_find->second;
                pathToResolve = &mappedPath;
                
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                    .Msg("Resolver::_Resolve('%s') - Mapped to '%s'\n", 
                        assetPath.c_str(),
                        pathToResolve->c_str());

                break;
            }
        }
    }

    if (_IsAyonPath(*pathToResolve)) {
        for (const AyonUsdResolverContext* ctx: contexts) {
            if (!ctx) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                    .Msg("Resolver::_Resolve: Context is null, skipping\n");
                continue;
            }

            std::shared_ptr<ResolverContextCache> resolverCache = ctx->GetCachePtr();
            if (!resolverCache) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                    .Msg("Resolver::_Resolve: Context has no cache, skipping\n");
                continue;
            }

            AssetIdentifier* asset = nullptr;
            std::string cleanAssetPath = *pathToResolve;
            RES_FUNCS_REMOVE_SDF_ARGS(cleanAssetPath);
            asset = resolverCache->getAsset(cleanAssetPath, CacheName::AYONCACHE, true);

            size_t pos = pathToResolve->find(cleanAssetPath);
            std::string sdfArgs;
            if (pos != std::string::npos) {
                sdfArgs = pathToResolve->substr(pos + cleanAssetPath.length());
            }
            std::string resolvedPathStr = asset->getResolvedAssetPath().GetPathString() + sdfArgs;
            ArResolvedPath resolvedPath(resolvedPathStr);
            
            if (resolvedPath) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                    .Msg("Resolver::_Resolve( '%s' ) resolved \n", resolvedPath.GetPathString().c_str());
                return resolvedPath;
            }

            TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                .Msg("Resolver::_Resolve( '%s' ) not found in this context, trying next\n", pathToResolve->c_str());
        }

        TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                .Msg("Resolver::_Resolve( '%s' ) AYON URI could not be resolved\n", pathToResolve->c_str());
        return ArResolvedPath(*pathToResolve);
    }

    if (_IsRelativePath(*pathToResolve)) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                .Msg("Resolver::_Resolve( '%s' ) is relative path\n", pathToResolve->c_str());
        ArResolvedPath resolvedPath = _ResolveAnchored(ArchGetCwd(), *pathToResolve);
        if (resolvedPath) {
            return resolvedPath;
        }

        return ArResolvedPath(*pathToResolve);
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER)
                .Msg("Resolver::_Resolve( '%s' ) is absolute path - test\n", pathToResolve->c_str());

    return _ResolveAnchored(std::string(), *pathToResolve);
}

ArResolvedPath
AyonUsdResolver::_ResolveForNewAsset(const std::string &assetPath) const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_ResolveForNewAsset('%s')\n", assetPath.c_str());
    return ArResolvedPath(assetPath.empty() ? assetPath : TfAbsPath(assetPath));
}

ArResolverContext
AyonUsdResolver::_CreateDefaultContext() const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("Resolver::_CreateDefaultContext()\n");
    
    const AyonUsdResolverContext* currentCtx = _GetCurrentContextPtr();
    
    if (currentCtx) {
        const auto& mappings = currentCtx->GetMappingPairs();
        if (!mappings.empty()) {
            // Sync bound context to fallback
            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("Resolver::_CreateDefaultContext - Syncing bound context (%zu mappings) to fallback\n",
                     mappings.size());
            const_cast<AyonUsdResolver*>(this)->_fallbackContext = *currentCtx;
        }
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("Resolver::_CreateDefaultContext - Using bound context with %zu mappings\n",
                 mappings.size());
        return ArResolverContext(*currentCtx);
    }
    
    // No bound context - Configure Stage disconnected, clear fallback
    if (!_fallbackContext.GetMappingPairs().empty()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("Resolver::_CreateDefaultContext - No bound context, clearing fallback mappings (was %zu)\n",
                 _fallbackContext.GetMappingPairs().size());
        const_cast<AyonUsdResolver*>(this)->_fallbackContext.ClearMappingPairs();
    }
    
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("Resolver::_CreateDefaultContext - Returning empty fallback context\n");
    return ArResolverContext(_fallbackContext);
}

ArResolverContext
AyonUsdResolver::_CreateDefaultContextForAsset(const std::string &assetPath) const {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("Resolver::_CreateDefaultContextForAsset('%s')\n", assetPath.c_str());

    // Check if there's a currently bound context first
    const AyonUsdResolverContext* currentCtx = _GetCurrentContextPtr();
    if (currentCtx && !currentCtx->GetMappingPairs().empty()) {
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("Resolver::_CreateDefaultContextForAsset - Using bound context with %zu mappings\n",
                 currentCtx->GetMappingPairs().size());
        return ArResolverContext(*currentCtx);
    }

    // Check if assetPath is a mapping file (.usd or .json)
    if (!assetPath.empty() && 
        (assetPath.find(".usd") != std::string::npos || 
         assetPath.find(".json") != std::string::npos)) {
        
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("Resolver::_CreateDefaultContextForAsset - Checking if file contains mappings: '%s'\n", 
                 assetPath.c_str());
        
        // Try to load mappings from the file
        AyonUsdResolverContext tempContext(assetPath);
        
        // Only update fallback context if we actually loaded mappings
        if (!tempContext.GetMappingPairs().empty()) {
            TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                .Msg("Resolver::_CreateDefaultContextForAsset - File has mappings, updating fallback context with %zu mappings\n",
                     tempContext.GetMappingPairs().size());
            
            const_cast<AyonUsdResolver*>(this)->_fallbackContext = tempContext;
            return ArResolverContext(_fallbackContext);
        }
        
        // File has no mappings, preserve existing fallback context
        TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
            .Msg("Resolver::_CreateDefaultContextForAsset - File has no mappings, preserving existing fallback context with %zu mappings\n",
                 _fallbackContext.GetMappingPairs().size());
    }

    // Return existing fallback context (don't overwrite it)
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("Resolver::_CreateDefaultContextForAsset - Using existing fallback context with %zu mappings\n",
             _fallbackContext.GetMappingPairs().size());
    
    return ArResolverContext(_fallbackContext);
}

bool
AyonUsdResolver::_IsContextDependentPath(const std::string &assetPath) const {
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
    return &_fallbackContext;
}

PXR_NAMESPACE_CLOSE_SCOPE
