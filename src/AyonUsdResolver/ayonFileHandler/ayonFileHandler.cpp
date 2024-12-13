#include "ayonFileHandler.h"

#include <fstream>

#include <cstdlib>
#include <cstring>
#include <ynput/tool/ayon/rootHelpers.hpp>

PXR_NAMESPACE_USING_DIRECTIVE
// TODO pinning file hanlder should construct its cache directly at construction getAssetData should not call
// rootReplace
ayonDataFileHandler::ayonDataFileHandler(const std::string &pinningFilePath,
                                         const std::unordered_map<std::string, std::string> &rootReplaceData):
    m_pinningFilePath(pinningFilePath),
    m_rootReplaceData(rootReplaceData) {
    std::ifstream pinningFile(this->m_pinningFilePath);

    if (!pinningFile.is_open()) {
        throw std::runtime_error("ayonDataFileHandler was not able to open PinningFile: "
                                 + this->m_pinningFilePath.string());
    }

    nlohmann::json raw_pinning_file;
    try {
        raw_pinning_file = nlohmann::json::parse(pinningFile);
    }
    catch (const nlohmann::json::parse_error &e) {
        throw std::runtime_error("The pining File is not in the Correct Format: ");
    }

    nlohmann::json pinningData = raw_pinning_file.at("ayon_resolver_pinning_data");
    pinningData.erase("ayon_pinning_data_entry_scene");

    for (auto &entry: pinningData.items()) {
        std::string pathed_key = ynput::tool::ayon::rootReplace(entry.key(), this->m_rootReplaceData);
        std::string pathed_val = ynput::tool::ayon::rootReplace(entry.value(), this->m_rootReplaceData);
        this->m_pinningFileData[pathed_key] = pathed_val;
    }
};

/**
 * @brief return assetIdent populated with root rootReplaceData from the pinning file using the pinning file data loaded
 * at construction and the PROJECT_ROOTS env variable.
 * this is not a cached function it will reconstruct the assetIdent. it will not reload the file or the env var however.
 *
 * @param resolveKey UsdAssetIdent
 * @return populated assetIdent if key was found in pinning file. Empty assetIdent if key was not found
 */
assetIdent
ayonDataFileHandler::getAssetData(const std::string &resolveKey) {
    assetIdent assetEntry;

    std::string pinnedAssetPath;
    try {
        pinnedAssetPath = this->m_pinningFileData.at(resolveKey);
    }
    catch (const nlohmann::json::out_of_range &e) {
        return assetEntry;
    }

    if (!pinnedAssetPath.empty()) {
        assetEntry.setAssetIdentifier(resolveKey);
        assetEntry.setResolvedAssetPath(pinnedAssetPath);
    }

    return assetEntry;
};
