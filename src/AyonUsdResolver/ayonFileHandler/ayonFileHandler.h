#ifndef AR_AYON_FILE_HANDLER_H
#define AR_AYON_FILE_HANDLER_H

#include <filesystem>

#include <string>
#include <unordered_map>

#include "../cache/assetIdentDef.h"
#include <nlohmann/json.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

// TODO rename back to pinning file handler
class ayonDataFileHandler {
    public:
        ayonDataFileHandler(const std::string &pinningFilePath,
                            const std::unordered_map<std::string, std::string> &rootReplaceData);
        ~ayonDataFileHandler() = default;

        assetIdent getAssetData(const std::string &resolveKey);

    private:
        std::filesystem::path m_pinningFilePath;
        nlohmann::json m_pinningFileData;

        std::unordered_map<std::string, std::string> m_rootReplaceData;
};

#endif
