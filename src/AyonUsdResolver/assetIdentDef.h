#ifndef ASSET_IDENT_DEF
    #include "pxr/usd/ar/resolvedPath.h"

PXR_NAMESPACE_USING_DIRECTIVE

struct assetIdent {
        ArResolvedPath resolvedAssetPath;
        std::string assetIdentifier;
        bool empty();
};

#endif   // !ASSET_IDENT_DEF
