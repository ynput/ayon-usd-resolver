#include "pxr/pxr.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/writableAsset.h"
#include "pxr/base/vt/value.h"

#include <memory>
#include <string>



class YON_AssetResolver final : public PXR_NS::ArResolver
{
public:
    YON_AssetResolver();
    virtual ~YON_AssetResolver();

protected:

    //---IDENTIFIERS---
    std::string _CreateIdentifier(const std::string& assetPath,  const PXR_NS::ArResolvedPath& anchorAssetPath) const final;    
    std::string _CreateIdentifierForNewAsset(const std::string& assetPath, const PXR_NS::ArResolvedPath& anchorAssetPath) const final;

    //---RESOLVE---
    PXR_NS::ArResolvedPath _Resolve(const std::string& assetPath) const final;
    PXR_NS::ArResolvedPath _ResolveForNewAsset(const std::string& assetPath) const final;

    //---ASSET
    std::shared_ptr<PXR_NS::ArAsset> _OpenAsset(const PXR_NS::ArResolvedPath& resolvedPath) const final;
    std::shared_ptr<PXR_NS::ArWritableAsset> _OpenAssetForWrite(const PXR_NS::ArResolvedPath& resolvedPath, WriteMode writeMode) const final;    
};