#include "prewarm.h"

#include "../cache/resolverContextCache.h"
#include "../codes/debugCodes.h"
#include "../helpers/resolutionFunctions.h"
#include "../resolverContext.h"

#include <pxr/pxr.h>
#include <pxr/base/tf/debug.h>
#include <pxr/usd/ar/resolvedPath.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/layer.h>

#include <deque>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

const char* kSdfArgsMarker = ":SDF_FORMAT_ARGS";

// Split an asset identifier into (clean identifier, SDF format args suffix).
// e.g. "foo.usd:SDF_FORMAT_ARGS:layer_id=model&order=100"
//   -> {"foo.usd", ":SDF_FORMAT_ARGS:layer_id=model&order=100"}
// The clean part is what the resolver keys the cache on (USD strips the args
// before calling _Resolve); the suffix must be re-attached when OPENING the
// layer so prewarm touches the exact same SdfLayer identity composition will,
// rather than a phantom args-less layer.
void
splitSdfArgs(const std::string &identifier, std::string &clean, std::string &argsSuffix) {
    const size_t pos = identifier.find(kSdfArgsMarker);
    if (pos == std::string::npos) {
        clean = identifier;
        argsSuffix.clear();
    } else {
        clean = identifier.substr(0, pos);
        argsSuffix = identifier.substr(pos);
    }
}

// Only recurse into deps that SdfLayer can open as a layer (tolerating a trailing
// :SDF_FORMAT_ARGS suffix). Asset-path attributes pointing at textures etc. are
// resolved by the batch but never scanned for further composition deps.
bool
looksLikeLayer(const std::string &path) {
    static const char *exts[] = {".usd", ".usda", ".usdc", ".usdz"};
    for (const char *e: exts) {
        const std::string ext(e);
        if (path.size() >= ext.size() && path.compare(path.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
        if (path.find(ext + ":") != std::string::npos) {
            return true;
        }
    }
    return false;
}

}   // namespace

void
AyonUsdResolverPrewarmStage(const std::string &rootAssetPath) {
    std::shared_ptr<ResolverContextCache> cache = GetResolverGlobalCache();
    if (rootAssetPath.empty() || !cache) {
        return;
    }
    if (cache->isCacheStatic()) {
        // Pinning mode already resolves locally; there is nothing to warm.
        return;
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("Prewarm: starting BFS from '%s'\n", rootAssetPath.c_str());

    ArResolver &resolver = ArGetResolver();

    std::unordered_set<std::string> scheduledUris;    // clean AYON URIs already batched
    std::unordered_set<std::string> openedLayers;     // layer identifiers already queued (with args)
    std::deque<std::string> layerFrontier;            // openable layer identifiers to scan (with args)

    // The root is handed to us as the asset path USD is about to open. Resolve it
    // through the resolver (a single round-trip at worst if it is an AYON URI; a
    // no-op for an already-local file path) so we can read it as a layer.
    {
        std::string rootClean;
        std::string rootArgs;
        splitSdfArgs(rootAssetPath, rootClean, rootArgs);
        ArResolvedPath rootResolved = resolver.Resolve(rootClean);
        std::string rootResolvedStr = rootResolved.GetPathString();
        if (rootResolvedStr.empty()) {
            rootResolvedStr = rootClean;
        }
        rootResolvedStr += rootArgs;
        layerFrontier.push_back(rootResolvedStr);
        openedLayers.insert(rootResolvedStr);
    }

    size_t batchRounds = 0;
    size_t totalResolved = 0;

    while (!layerFrontier.empty()) {
        std::deque<std::string> currentFrontier;
        currentFrontier.swap(layerFrontier);

        // Clean AYON URIs to batch this round, each paired with the SDF args suffix
        // that must be re-attached to the resolved path before we open it.
        std::vector<std::string> uriFrontier;
        std::vector<std::pair<std::string, std::string>> scheduledThisRound;   // (cleanUri, argsSuffix)
        std::vector<std::string> localLayerFrontier;                           // non-AYON layers to scan next

        for (const std::string &layerPath: currentFrontier) {
            SdfLayerRefPtr layer = SdfLayer::FindOrOpen(layerPath);
            if (!layer) {
                TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
                    .Msg("Prewarm: could not open layer '%s'\n", layerPath.c_str());
                continue;
            }

            // Anchor relative deps against the layer's real (args-less) path.
            std::string anchorClean;
            std::string anchorArgs;
            splitSdfArgs(layerPath, anchorClean, anchorArgs);

            for (const std::string &dep: layer->GetCompositionAssetDependencies()) {
                if (dep.empty()) {
                    continue;
                }

                // The full identifier (with any :SDF_FORMAT_ARGS) as composition sees it.
                std::string identFull = resolver.CreateIdentifier(dep, ArResolvedPath(anchorClean));
                std::string identClean;
                std::string argsSuffix;
                splitSdfArgs(identFull, identClean, argsSuffix);

                if (_IsAyonPath(identClean)) {
                    // Cache key is the clean URI (what _Resolve looks up); remember the
                    // args so we open the matching layer identity, not a phantom args-less one.
                    if (scheduledUris.insert(identClean).second) {
                        uriFrontier.push_back(identClean);
                    }
                    scheduledThisRound.emplace_back(identClean, argsSuffix);
                    continue;
                }

                // Local/relative sublayer: resolving it is a cheap filesystem op.
                // Queue it (with its args) so we discover any AYON refs nested beneath it.
                ArResolvedPath localResolved = resolver.Resolve(identClean);
                std::string localResolvedStr = localResolved.GetPathString();
                if (!localResolvedStr.empty()) {
                    localResolvedStr += argsSuffix;
                    if (looksLikeLayer(localResolvedStr) && openedLayers.insert(localResolvedStr).second) {
                        localLayerFrontier.push_back(localResolvedStr);
                    }
                }
            }
        }

        // One batched resolve for the entire AYON frontier (clean keys only).
        std::unordered_map<std::string, std::string> resolved;
        if (!uriFrontier.empty()) {
            ++batchRounds;
            resolved = cache->batchWarm(uriFrontier);
            totalResolved += resolved.size();
        }

        // Queue each resolved AYON layer for the next BFS level, re-attaching its
        // SDF args so we open exactly the layer identity composition will use.
        for (const auto &scheduled: scheduledThisRound) {
            const std::string &cleanUri = scheduled.first;
            const std::string &argsSuffix = scheduled.second;
            auto it = resolved.find(cleanUri);
            if (it == resolved.end() || it->second.empty()) {
                continue;
            }
            std::string openable = it->second + argsSuffix;
            if (looksLikeLayer(openable) && openedLayers.insert(openable).second) {
                layerFrontier.push_back(openable);
            }
        }

        for (const std::string &localLayer: localLayerFrontier) {
            layerFrontier.push_back(localLayer);
        }
    }

    TF_DEBUG(AYONUSDRESOLVER_RESOLVER_CONTEXT)
        .Msg("Prewarm: done - %zu uris resolved across %zu batched round(s)\n", totalResolved, batchRounds);
}

extern "C" void
AyonUsdResolverPrewarm(const char *rootAssetPath) {
    if (rootAssetPath == nullptr) {
        return;
    }
    AyonUsdResolverPrewarmStage(std::string(rootAssetPath));
}
