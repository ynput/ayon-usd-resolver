#include <cstdint>
#include <iostream>
#include <ostream>
#include <regex>

#include <string_view>
#include <string_view>

#include "resolutionFunctions.h"
#include "../codes/debugCodes.h"
#include "../config.h"

#include "pxr/base/tf/debug.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/ar/filesystemAsset.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

void
RemoveSdfFormatArgs(std::string &uri) {
    if (uri.empty()) {
        return;
    }

    std::regex sdfArgsRegex(":SDF_FORMAT_ARGS.*");
    std::string clean_uri = std::regex_replace(uri, sdfArgsRegex, "");
    uri = clean_uri;
}

bool
_IsRelativePath(const std::string &path) {
    return (!path.empty() && TfIsRelativePath(path));
}

bool
_IsFileRelativePath(const std::string &path) {
    return path.find("./") == 0 || path.find("../") == 0;
}
bool
_IsAyonPath(const std::string &assetPath) {
    TF_DEBUG(AYONUSDRESOLVER_RESOLVER).Msg("Resolver::_IsAyonPath (%s) \n", assetPath.c_str());
    // TODO this constrcution should not be needed
    Config::AyonUriConfigStruct config;
    for (uint8_t i = 0; i < config.ayonUriOptions.size(); i++) {
        std::string_view assetPathTestPortion(assetPath.data(), config.ayonUriOptionsSize.at(i));

        if (assetPathTestPortion == config.ayonUriOptions.at(i)) {
            return true;
        }
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
    // Ensure we are using forward slashes and not backslashes.
    std::string forwardPath = anchorPath;
    std::replace(forwardPath.begin(), forwardPath.end(), '\\', '/');
    // If anchorPath does not end with '/', we assume it is specifying
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
