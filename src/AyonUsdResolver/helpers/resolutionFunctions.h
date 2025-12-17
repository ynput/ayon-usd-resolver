// #ifndef AR_AYONUSDRESOLVER_RESELUTIONFUNCTIONS_H
// #define AR_AYONUSDRESOLVER_RESELUTIONFUNCTIONS_H

#include "pxr/pxr.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include <string>

#define CONVERT_STRING(string) #string
#define DEFINE_STRING(string)  CONVERT_STRING(string)

PXR_NAMESPACE_USING_DIRECTIVE

void RemoveSdfFormatArgs(std::string &uri);

#if MACRO_REMOVE_SDF_FORMAT_ARGS == 1
    #define RES_FUNCS_REMOVE_SDF_ARGS(x) RemoveSdfFormatArgs(x);
#else
    #define RES_FUNCS_REMOVE_SDF_ARGS(x) ;
#endif

bool _IsRelativePath(const std::string &path);

bool _IsFileRelativePath(const std::string &path);

bool _IsAyonPath(const std::string &assetPath);

bool _IsNotFilePath(const std::string &path);

std::string _AnchorRelativePath(const std::string &anchorPath, const std::string &path);

ArResolvedPath _ResolveAnchored(const std::string &anchorPath, const std::string &path);

// #endif   // AR_AYONUSDRESOLVER_RESELUTIONFUNCTIONS_H
