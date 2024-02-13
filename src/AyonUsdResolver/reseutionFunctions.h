// #ifndef AR_AYONUSDRESOLVER_RESELUTIONFUNCTIONS_H
// #define AR_AYONUSDRESOLVER_RESELUTIONFUNCTIONS_H

#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include <string_view>
#include <string>

#define CONVERT_STRING(string) #string
#define DEFINE_STRING(string)  CONVERT_STRING(string)

PXR_NAMESPACE_USING_DIRECTIVE

bool _IsRelativePath(const std::string &path);

bool _IsFileRelativePath(const std::string &path);

// TODO give this a more generalised name
bool _IsAyonPath(const std::string &assetPath, const std::string_view &uriIdent, const uint8_t &uriIdentSize);

bool _IsNotFilePath(const std::string &path);

std::string _AnchorRelativePath(const std::string &anchorPath, const std::string &path);

ArResolvedPath _ResolveAnchored(const std::string &anchorPath, const std::string &path);

bool _IsPinningFile(const std::string &assetPath);

// #endif   // AR_AYONUSDRESOLVER_RESELUTIONFUNCTIONS_H
