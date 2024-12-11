#ifndef AR_AYON_REDIRECTION_HANDLER_H
#define AR_AYON_REDIRECTION_HANDLER_H

// TODO the redirectionFile class was implemented with the idea that we will allways load data from a file when a class
// is created. now where this is not the case we need to refactor it to allow for better perfromance because right now
// every resolver context has a redirection file attached and therbey the if nullptr check is not usefull.
#include <filesystem>

#include <optional>
#include <shared_mutex>
#include <string>

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../pluginData/api.h"

/**
 * @brief allows the loading of Redirection Files and acces to the composed Redirection Data
 *
 * @return
 */

class redirectionFile {
    public:
        AR_AYONUSDRESOLVER_API
        redirectionFile();
        AR_AYONUSDRESOLVER_API
        redirectionFile(const std::string &entryFile);
        AR_AYONUSDRESOLVER_API
        redirectionFile(const std::filesystem::path &entryFile);
        AR_AYONUSDRESOLVER_API
        ~redirectionFile();

        AR_AYONUSDRESOLVER_API
        redirectionFile(const redirectionFile &) = delete;
        AR_AYONUSDRESOLVER_API
        redirectionFile &operator=(const redirectionFile &) = delete;
        AR_AYONUSDRESOLVER_API
        redirectionFile(redirectionFile &&) = delete;
        AR_AYONUSDRESOLVER_API
        redirectionFile &operator=(redirectionFile &&) = delete;
        AR_AYONUSDRESOLVER_API
        void init(const std::filesystem::path &entryFile);

        /**
         * @brief gets the Redirection data from a given data Key
         *
         * @param key
         * @return
         */
        AR_AYONUSDRESOLVER_API
        std::optional<std::pair<std::string, std::string>> getRedirectionForKey(const std::string &key) const;
        /**
         * @brief returns a refernece to the internal layers.
         */
        AR_AYONUSDRESOLVER_API
        const std::vector<std::filesystem::path> &getLayers() const;

        AR_AYONUSDRESOLVER_API
        std::vector<std::string> getLayersStr() const;
        /**
         * @brief adds a layer to the root layer
         *
         * @param layerPath the sublayer to be added
         * @param strongOrder decide if the layer is added as the strongest sublayer or the weakest
         * @return
         */
        AR_AYONUSDRESOLVER_API
        bool addLayer(const std::filesystem::path &layerPath, bool strongOrder);

        /**
         * @brief adds a new layer to the sublayers and reloads the RDF_File
         *
         * @param layerPath path to the layer in the way it should appear in the sblayers
         * @param strongOrder decide if this layer is the strongest or weakest layer in the sublayers
         * @return true if all operatoins where sucsessfull
         */
        AR_AYONUSDRESOLVER_API
        bool addLayerStr(const std::string &layerPath, bool strongOrder);
        /**
         * @brief reloads the RDF_File, this causes recursive read on the disk and a full recreation off all data point
         *
         * @return
         */
        AR_AYONUSDRESOLVER_API
        bool reload();
        /**
         * @brief helper funciton that prints all the data to std out
         */
        AR_AYONUSDRESOLVER_API
        void printData();
        /**
         * @brief saves the current state of the root layer file to disk
         *
         * @return
         */
        AR_AYONUSDRESOLVER_API
        bool save();

        /**
         * @brief saves the rdf to a given file and sets the m_loadedLayers[0] aka the root layer to the end path
         *
         * @param savePath
         * @return
         */
        AR_AYONUSDRESOLVER_API
        bool saveToFile(const std::string &savePath);
        /**
         * @brief adds a redirection entry to the current root layer
         *
         * @param key
         * @param val
         * @return
         */
        AR_AYONUSDRESOLVER_API
        bool addRedirection(const std::string &key, const std::string &val);

        AR_AYONUSDRESOLVER_API
        bool
        operator==(const redirectionFile &rdf) const {
            return m_redirectionData == rdf.m_redirectionData && m_loadedLayers == rdf.m_loadedLayers
                   && m_subLayers == rdf.m_subLayers && m_internalData == rdf.m_internalData;
        }
        AR_AYONUSDRESOLVER_API
        bool
        operator!=(const redirectionFile &rdf) const {
            return !(*this == rdf);
        }

    private:
        /**
         * @brief loads the defined layerStack and writes this data into m_inputFiles
         *
         * @return
         */

        bool getLayerStack(const std::filesystem::path &entryFile);

        /**
         * @brief reads the data section from the files in the layer stack into  m_redirectionData
         *
         * @return
         */
        bool readLayerStackData();
        std::filesystem::path entryFile;

        mutable std::shared_mutex m_redirectionDataMutex;
        std::unordered_map<std::string, std::string> m_redirectionData;

        mutable std::shared_mutex m_loadedLayersMutex;
        std::vector<std::filesystem::path> m_loadedLayers;

        mutable std::shared_mutex m_subLayersMutex;
        std::vector<std::filesystem::path> m_subLayers;

        mutable std::shared_mutex m_internalDataMutex;
        std::unordered_map<std::string, std::string> m_internalData;
};

AR_AYONUSDRESOLVER_API std::pair<redirectionFile*, std::string> getRdFile();
AR_AYONUSDRESOLVER_API redirectionFile* getRdFile(const std::filesystem::path &entryFile);

#endif
