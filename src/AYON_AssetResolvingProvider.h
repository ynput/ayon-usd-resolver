#include <string>
#include "AYON_AssetResolveResult.h"

class AYON_AssetResolvingProvider
{
public:
    virtual AYON_AssetResolveResult Resolve(const std::string& AssetPath) = 0;   
};