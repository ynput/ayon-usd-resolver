#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <filesystem>

#include "nlohmann/json_fwd.hpp"
#include <nlohmann/json.hpp>

#include "redirectionHanlder.h"
#include <sys/types.h>
#include "../helpers/resolutionFunctions.h"
#include "AyonCppApi.h"

std::shared_mutex _redirectionFileSingeltonMutex;
std::unordered_map<std::filesystem::path, std::unique_ptr<redirectionFile>> _redirectionFileSingelton;

std::filesystem::path
getTmpFilePath(const std::string &filePath) {
    return std::filesystem::temp_directory_path() / std::filesystem::path(filePath);
};

std::string
generateRandomID(size_t length) {
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, characters.size() - 1);

    std::string randomID;
    std::generate_n(std::back_inserter(randomID), length, [&]() { return characters[dist(gen)]; });

    return randomID;
}

std::pair<redirectionFile*, std::string>
getRdFile() {
    std::filesystem::path randId(generateRandomID(64));

    std::shared_lock<std::shared_mutex> RLock(_redirectionFileSingeltonMutex);
    std::unordered_map<std::filesystem::path, std::unique_ptr<redirectionFile>>::iterator it
        = _redirectionFileSingelton.find(randId);
    if (it != _redirectionFileSingelton.end()) {
        return {it->second.get(), randId.string()};
    }
    RLock.unlock();

    std::unique_lock<std::shared_mutex> WLock(_redirectionFileSingeltonMutex);
    const auto [emplacedIt, success] = _redirectionFileSingelton.emplace(randId, std::make_unique<redirectionFile>());

    return {emplacedIt->second.get(), randId.string()};
};

// TODO error when loading an empty file because of parsing error
redirectionFile*
getRdFile(const std::filesystem::path &entryFile) {
    std::shared_lock<std::shared_mutex> RLock(_redirectionFileSingeltonMutex);
    std::unordered_map<std::filesystem::path, std::unique_ptr<redirectionFile>>::iterator it
        = _redirectionFileSingelton.find(entryFile);
    if (it != _redirectionFileSingelton.end()) {
        return it->second.get();
    }
    RLock.unlock();

    std::unique_lock<std::shared_mutex> WLock(_redirectionFileSingeltonMutex);
    const auto [emplacedIt, success]
        = _redirectionFileSingelton.emplace(entryFile, std::make_unique<redirectionFile>(entryFile));
    return emplacedIt->second.get();
}

redirectionFile::redirectionFile() {
    std::string rand(generateRandomID(64));
    std::cout << "randId rdf" << rand << "\n";
    m_loadedLayers.push_back(getTmpFilePath(rand + ".json"));
    this->init(m_loadedLayers.at(0));
};

redirectionFile::redirectionFile(const std::filesystem::path &entryFile) {
    this->init(entryFile);
};

redirectionFile::redirectionFile(const std::string &entryFile) {
    std::filesystem::path entryFilePath(entryFile);
    this->init(entryFilePath);
};

redirectionFile::~redirectionFile() {
    std::cout << "redirectionFile::~redirectionFile(" << m_loadedLayers.at(0) << ")" << std::endl;
};

void
redirectionFile::init(const std::filesystem::path &entryFile) {
    std::cout << "redirectionFile: " << entryFile << std::endl;
    if (!std::filesystem::exists(entryFile)) {
        std::cout << "Warn: Cant find redirectionFile. Redirection File will be created" << std::endl;
        std::ofstream ofs(entryFile);
        nlohmann::json rdfJson;
        rdfJson["subLayers"] = nlohmann::json::array();
        rdfJson["data"] = {};
        ofs << rdfJson.dump(4);
    }
    if (!this->getLayerStack(entryFile)) {
        std::cout << "Cant Get Layer Stack" << std::endl;
    }

    std::cout << std::endl;

    if (!this->readLayerStackData()) {
        std::cout << "Cant Read data from LayerStack" << std::endl;
    }
    // TODO this violates the DRY principle and also causes 1 more read
    std::ifstream ifs(entryFile);
    nlohmann::json entryJson = nlohmann::json::parse(ifs);

    std::unique_lock<std::shared_mutex> WLock(m_subLayersMutex);
    for (const std::string &layerIdent: entryJson.at("subLayers")) {
        std::filesystem::path layerFile(layerIdent);
        if (!layerFile.is_absolute()) {
            layerFile = std::filesystem::absolute(entryFile.parent_path() / layerFile);
        }
        this->m_subLayers.push_back(layerIdent);
    }
}

// TODO make this return a Pair
std::optional<std::pair<std::string, std::string>>
redirectionFile::getRedirectionForKey(const std::string &key) const {
    std::shared_lock<std::shared_mutex> RLock(m_redirectionDataMutex);
    std::unordered_map<std::string, std::string>::const_iterator hit = this->m_redirectionData.find(key);
    if (hit != this->m_redirectionData.end()) {
        return std::make_pair(hit->first, hit->second);
    };

    return std::nullopt;
}

bool
redirectionFile::readLayerStackData() {
    std::shared_lock<std::shared_mutex> RLock(m_loadedLayersMutex);
    std::unique_lock<std::shared_mutex> WLock(m_redirectionDataMutex);
    for (std::vector<std::filesystem::path>::reverse_iterator it = this->m_loadedLayers.rbegin();
         it != this->m_loadedLayers.rend(); it++) {
        std::ifstream ifs(it->string());
        nlohmann::json entryJson = nlohmann::json::parse(ifs);
        for (const auto &entry: entryJson.at("data").items()) {
            if (_IsAyonPath(entry.value())) {
                AyonApi api;
                std::pair<std::string, std::string> asset = api.resolvePath(entry.value());

                this->m_redirectionData[entry.key()] = asset.second;
                continue;
            }
            if (std::filesystem::exists(entry.value())) {
                this->m_redirectionData[entry.key()] = entry.value();
                continue;
            }
            this->m_redirectionData[entry.key()] = std::filesystem::weakly_canonical(
                std::filesystem::absolute(std::filesystem::path(it->string()).parent_path()
                                          / std::filesystem::path(entry.value().get<std::string>())));
        }
    }

    return true;
};

bool
redirectionFile::getLayerStack(const std::filesystem::path &entryFile) {
    std::unique_lock<std::shared_mutex> WLock(m_loadedLayersMutex);

    if (std::find(m_loadedLayers.begin(), m_loadedLayers.end(), entryFile) != m_loadedLayers.end()) {
        std::cout << "getLayerStack //Layer File was found in a higher Layer: " << entryFile << std::endl;
    }
    else {
        this->m_loadedLayers.push_back(entryFile);
    }

    std::ifstream ifs(entryFile);
    nlohmann::json entryJson = nlohmann::json::parse(ifs);
    WLock.unlock();

    for (const auto &layerIdent: entryJson.at("subLayers")) {
        std::filesystem::path layerFile(layerIdent);
        if (!layerFile.is_absolute()) {
            layerFile = std::filesystem::absolute(entryFile.parent_path() / layerFile);
        }

        if (!std::filesystem::exists(layerFile)) {
            std::cout << "Warn: cant find RedirectoinFile: " << layerFile.string() << "\n" << std::endl;
            continue;
        }

        this->getLayerStack(layerFile);
    }
    return true;
};

const std::vector<std::filesystem::path> &
redirectionFile::getLayers() const {
    return m_loadedLayers;
};

std::vector<std::string>
redirectionFile::getLayersStr() const {
    std::shared_lock<std::shared_mutex> RLock(m_loadedLayersMutex);
    std::vector<std::string> rdfLayers;
    for (const std::filesystem::path &layerPath: this->m_loadedLayers) {
        rdfLayers.push_back(layerPath.string());
    }
    return rdfLayers;
};

bool
redirectionFile::addLayer(const std::filesystem::path &layerPath, bool strongOrder) {
    // add the layer to our sublayers
    // check if the layer is the strongest or the weakest
    // if the layer is strongest overwrite the data present if its not in the root file
    // if its weakest only load the data if its not present
    std::unique_lock<std::shared_mutex> WLock(m_subLayersMutex);
    std::vector<std::filesystem::path>::iterator hit = std::find(m_subLayers.begin(), m_subLayers.end(), layerPath);

    if (hit != m_subLayers.end()) {
        return true;
    }
    if (strongOrder) {
        m_subLayers.insert(m_subLayers.begin(), layerPath);
    }
    else {
        m_subLayers.push_back(layerPath);
    }

    WLock.unlock();

    this->save();

    std::shared_lock<std::shared_mutex> RLock(m_loadedLayersMutex);
    std::filesystem::path rootLayer(this->m_loadedLayers.at(0));
    RLock.unlock();
    this->init(rootLayer);

    return true;
};

bool
redirectionFile::addLayerStr(const std::string &layerPath, bool strongOrder) {
    return this->addLayer(std::filesystem::path(layerPath), strongOrder);
};

bool
redirectionFile::save() {
    nlohmann::json entryJson;

    std::shared_lock<std::shared_mutex> RLock(m_subLayersMutex);
    std::shared_lock<std::shared_mutex> RLockB(m_internalDataMutex);

    std::shared_lock<std::shared_mutex> RLockLoadedLayers(m_loadedLayersMutex);
    if (!m_loadedLayers.at(0).is_absolute()) {
        throw std::runtime_error("Cant save to a file to" + m_loadedLayers.at(0).string());
    }

    entryJson["subLayers"] = this->m_subLayers;
    entryJson["data"] = this->m_internalData;

    std::ofstream ofs(m_loadedLayers.at(0));

    if (ofs) {
        ofs << entryJson.dump(4);
    }
    else {
        std::cout << "Cant open file for writing" << std::endl;
    }

    return true;
}

bool
redirectionFile::saveToFile(const std::string &savePath) {
    m_loadedLayers.at(0) = savePath;
    this->save();
    return true;
};

bool
redirectionFile::reload() {
    std::cout << "redirectionFile::reload" << std::endl;

    std::shared_lock<std::shared_mutex> RLockLoadedLayers(m_loadedLayersMutex);
    if (!m_loadedLayers.at(0).is_absolute()) {
        throw std::runtime_error("Cant save to a file to" + m_loadedLayers.at(0).string());
    }

    this->init(this->m_loadedLayers.at(0));
    return true;
};
void redirectionFile::printData() {};

bool
redirectionFile::addRedirection(const std::string &key, const std::string &val) {
    // Update the runtime Data
    // save the changes that need to be made to the root layer file
    std::string resolvedVal = val;
    if (!std::filesystem::path(resolvedVal).is_absolute()) {
        resolvedVal = std::filesystem::weakly_canonical(std::filesystem::path(m_loadedLayers.at(0).parent_path())
                                                        / std::filesystem::path(resolvedVal))
                          .string();
    }
    std::cout << resolvedVal << std::endl;

    m_redirectionData[key] = resolvedVal;
    m_internalData[key] = val;
    return true;
};
