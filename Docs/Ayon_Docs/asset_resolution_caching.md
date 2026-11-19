# Asset Resolution Flow with Caching

## Mermaid2 Diagram

```mermaid
flowchart TD
    A[Resolve assetPath] --> B{assetPath empty?}
    B -- yes --> Z1[Return empty ArResolvedPath]
    B -- no --> C[Pick context chain:\n 1. current bound context\n2. fallback context]

    C --> D{Mapping pair for assetPath exists?}
    D -- yes --> E[Use mappedPath]
    D -- no --> F[Use original assetPath]
    E --> G
    F --> G

    G{Is AYON URI?} -- no --> P{Relative path?}
    P -- yes --> Q[_ResolveAnchored cwd + path]
    P -- no --> R[_ResolveAnchored absolute/anchored path]
    Q --> Z2[Return resolved or passthrough path]
    R --> Z2

    G -- yes --> H[Iterate contexts and fetch context cache]
    H --> I[Strip SDF args from key]
    I --> J[getAsset key, AYONCACHE, true]

    subgraph CACHE_FLOW [ResolverContextCache getAsset cache emphasis]
        J --> S0{Static cache mode?}
        S0 -- yes --> S1[Pinning file lookup]
        S1 --> S2{Hit?}
        S2 -- yes --> SRET[Return pinned asset]
        S2 -- no --> SMISS[Return empty]

        S0 -- no --> K1{PreCache hit?}
        K1 -- yes --> KRET[Return PreCache asset]
        K1 -- no --> K2{AyonCache hit?}
        K2 -- yes --> KRET2[Return AyonCache asset]
        K2 -- no --> K3{Memcached enabled and connected?}
        K3 -- yes --> K4{Memcached hit?}
        K4 -- yes --> K5[Apply rootReplace to cached value]
        K5 --> K6[Insert into PreCache]
        K6 --> KRET3[Return memcached asset]
        K4 -- no --> K7[Call AYON API resolvePath]
        K3 -- no --> K7
        K7 --> K8[Insert into PreCache]
        K8 --> K9[Write-through to memcached]
        K9 --> KRET4[Return API asset]

        K8 --> M1{PreCache reached capacity?}
        M1 -- yes --> M2[Migrate PreCache to AyonCache]
        M1 -- no --> M3[Keep in PreCache]
    end

    SRET --> L
    SMISS --> L
    KRET --> L
    KRET2 --> L
    KRET3 --> L
    KRET4 --> L

    L[Re-append SDF args] --> N{Resolved path valid?}
    N -- yes --> Z3[Return resolved path]
    N -- no --> O[Try next context]

    O --> O2{More contexts?}
    O2 -- yes --> I
    O2 -- no --> Z4[Return unresolved AYON URI as ArResolvedPath]

    X[Process-wide shared cache instance] -. shared by .-> H
```

## Summary

- Resolution begins by evaluating the current context, then fallback context, and applying any mapping pair substitution before path-type logic.
- Non-AYON paths bypass AYON cache logic and resolve through anchored filesystem resolution.
- AYON paths use a cache-first strategy in this order: PreCache, AyonCache, memcached, then AYON REST API.
- Memcached acts as a distributed second-level cache:
  - On hit, values are root-replaced and promoted into local PreCache.
  - On miss, the resolver calls AYON API and writes through to memcached.
- Local hot data is kept in PreCache; when full, entries are migrated into AyonCache for longer-lived in-process reuse.
- In static cache mode, dynamic resolution is bypassed and pinning-file data is returned directly.
- A process-wide shared ResolverContextCache instance is used by contexts, which improves reuse across resolutions in the same process.
