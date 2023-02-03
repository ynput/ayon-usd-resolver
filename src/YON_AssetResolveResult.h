#pragma once
#include <string>

class YON_AssetResolveResult
{
public:
	std::string ResolvedPath = "___UNRESOLVED_PATH___";
	std::string ErrorMsg;
	bool bResolved = false;
};