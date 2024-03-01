#include "reseutionFunctions.h"
#include <cstdint>
#include <iostream>
#include <string_view>

#include <string_view>
#include "debugCodes.h"
#include "pxr/base/tf/debug.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "reseutionFunctions.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/ar/filesystemAsset.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

bool
_IsRelativePath(const std::string &path) {
    return (!path.empty() && TfIsRelativePath(path));
}

bool
_IsFileRelativePath(const std::string &path) {
    return path.find("./") == 0 || path.find("../") == 0;
}
// TODO the next 2 functions can be one function with an option to start at end or begin
bool
_IsAyonPath(const std::string &assetPath, const std::string_view &uriIdent, const uint8_t &uriIdentSize) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_IsAyonPath (%s) \n", assetPath.c_str());
    std::string_view ayonIdent(assetPath.data(), uriIdentSize);

    if (uriIdent == ayonIdent) {
        return true;
    }
    return false;
}

bool
_IsPinningFile(const std::string &assetPath) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_IsAyonPath (%s) \n", assetPath.c_str());
    std::string_view ayonPinningFileExt(".pin");
    std::string_view fileExtention(assetPath.data() + std::max<int>(0, assetPath.size() - 4),
                                   std::min<int>(4, assetPath.size()));
    if (ayonPinningFileExt == fileExtention) {
        return true;
    }
    return false;
}

bool
_IsNotFilePath(const std::string &path) {
    return _IsRelativePath(path) && !_IsFileRelativePath(path);
}

std::string
_AnchorRelativePath(const std::string &anchorPath, const std::string &path) {
    if (TfIsRelativePath(anchorPath) || !_IsRelativePath(path)) {
        return path;
    }
    // Ensure we are using forward slashes and not back slashes.
    std::string forwardPath = anchorPath;
    std::replace(forwardPath.begin(), forwardPath.end(), '\\', '/');
    // If anchorPath does not end with a '/', we assume it is specifying
    // a file, strip off the last component, and anchor the path to that
    // directory.
    const std::string anchoredPath = TfStringCatPaths(TfStringGetBeforeSuffix(forwardPath, '/'), path);
    return TfNormPath(anchoredPath);
}

ArResolvedPath
_ResolveAnchored(const std::string &anchorPath, const std::string &path) {
    std::string resolvedPath = path;
    if (!anchorPath.empty()) {
        resolvedPath = TfStringCatPaths(anchorPath, path);
    }
    return TfPathExists(resolvedPath) ? ArResolvedPath(TfAbsPath(resolvedPath)) : ArResolvedPath();
}
