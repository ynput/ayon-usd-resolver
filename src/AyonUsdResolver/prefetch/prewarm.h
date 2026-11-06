#ifndef AR_AYONUSDRESOLVER_PREWARM_H
#define AR_AYONUSDRESOLVER_PREWARM_H

#include <string>

/**
 * @brief Pre-resolve a stage's AYON URIs in batched requests before composition.
 *
 * Walks the composition graph rooted at rootAssetPath breadth-first using
 * SdfLayer::GetCompositionAssetDependencies() (local layer reads, no server
 * round-trips). Each BFS frontier of AYON URIs is resolved in a single batched,
 * parallel request and inserted into the process-wide resolver cache. By the time
 * USD composes the stage and calls _Resolve() per asset, the cache is already warm,
 * so the dozens of serial ~750ms round-trips collapse into one batched call per
 * graph level.
 *
 * Safe to call with a non-AYON or unreadable root (does nothing useful, never throws).
 * No-op when the cache is in static (pinning) mode.
 */
void AyonUsdResolverPrewarmStage(const std::string &rootAssetPath);

extern "C" {
/**
 * @brief C entry point for host-side (e.g. ctypes) invocation. Wraps
 * AyonUsdResolverPrewarmStage. Null/empty rootAssetPath is ignored.
 */
void AyonUsdResolverPrewarm(const char *rootAssetPath);
}

#endif   // AR_AYONUSDRESOLVER_PREWARM_H
