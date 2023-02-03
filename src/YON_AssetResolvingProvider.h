#include <string>
#include "YON_AssetResolveResult.h"

class YON_AssetResolvingProvider
{
public:
    virtual YON_AssetResolveResult Resolve(const std::string& AssetPath) = 0;   
};