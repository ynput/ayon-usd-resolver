#pragma once

#include "AyonCppApiCrossPlatformMacros.h"
#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <regex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>
#include "AyonLogger.h"
#include "appDataFoulder.h"
#include "httplib.h"
#include "nlohmann/json_fwd.hpp"

/**
 * @class AyonApi
 * @brief Central Ayon api class \n
 * This class exposes functions off the ayon api to the user while wrapping around httplib for requests
 *
 */
class AyonApi {
    public:
        AyonApi();
        ~AyonApi();

        /**
         * @brief returns the stored apikey
         */
        std::string getKey();
        /**
         * @brief returns the stored AYON server url
         */
        std::string getUrl();

        nlohmann::json GET(const std::string &endPoint);

        /**
         * @brief calls the server via a shared httplib client ( serial )
         *
         * @param endPoint the AYON enpoint to hit
         * @param headers the http header that you want to send
         * @param jsonPayload the payload in json format
         */
        nlohmann::json SPOST(const std::shared_ptr<std::string> endPoint,
                             const std::shared_ptr<httplib::Headers> headers,
                             nlohmann::json jsonPayload,
                             const std::shared_ptr<uint8_t> sucsessStatus);
        /**
         * @brief calls the AYON server while also creation a new httplib client ( Generative Async )
         *
         * @param endPoint the AYON enpoint to hit
         * @param headers the http header that you want to send
         * @param jsonPayload the payload in json format
         */
        nlohmann::json CPOST(const std::shared_ptr<std::string> endPoint,
                             const std::shared_ptr<httplib::Headers> headers,
                             nlohmann::json jsonPayload,
                             const std::shared_ptr<uint8_t> sucsessStatus);

        /**
         * @brief uses the uri resolve endpoint on the AYON server in order to resolve an uri path towards the local
         * path \n it gets the siteId from a varible stored in the class
         *
         * @param uriPath
         */
        std::pair<std::string, std::string> resolvePath(const std::string &uriPath);
        /**
         * @brief resolves manny paths against the AYON server in an async way
         *
         * @param uriPaths
         */
        std::unordered_map<std::string, std::string> batchResolvePath(std::vector<std::string> &uriPaths);

        /**
         * @brief this function takes an ayon:// path and returns a pair of assetIdentifier(ayon:// path) and the
         * machine local file location
         *
         * @param assetIdentifier
         */
        std::pair<std::string, std::string> getAssetIdent(const nlohmann::json &uriResolverRespone);

        /**
         * @brief this function loads all needed varible into the class \n
         * this will allso be called by the constructor
         *
         * @return
         */
        bool loadEnvVars();

    private:
        /**
         * @brief calls the server in an serial way by sharing the AyonServer pointer
         *
         * @param endPoint
         * @param headers
         * @param Payload
         */
        std::string serialCorePost(const std::string &endPoint,
                                   httplib::Headers headers,
                                   std::string &Payload,
                                   const int &sucsessStatus);
        /**
         * @brief calls the server while creating a new client instance to stay async
         *
         * @param endPoint
         * @param headers
         * @param Payload
         */
        std::string GenerativeCorePost(const std::string &endPoint,
                                       httplib::Headers headers,
                                       std::string &Payload,
                                       const int &sucsessStatus);

        /**
         * @brief converts a vector off uris into an string to serve into CorePost funcs
         *
         * @param uriVec
         */
        std::string convertUriVecToString(const std::vector<std::string> &uriVec);

        /**
         * @brief returns the version off a file when you give it an array off version (resolve version=* que)
         *
         * @param jsonData a single server response
         * @param versionNum optinal field that allows you to chose a version number (set to 65535 by default as its max
         * numver )
         */
        std::string getFileVersion(const nlohmann::json &jsonData, u_int16_t versionNum);

        /**
         * @brief splits the response group that Ayon uri resolve can return into individuale responses\n
         *
         * @param jsonDataGrp
         */
        std::vector<nlohmann::json> splitBatchResolveResponse(const nlohmann::json &jsonDataGrp);

        // ----- Env Varibles
        const char* authKey;
        const char* serverUrl;

        std::string ayonAppData = getAppDataDir() + "/AYON";

        // ---- Server Vars
        std::string siteId;
        std::string userName;

        // --- Runtime Dep Vars
        // Async Grp Generation Varibles
        u_int8_t minGrpSizeForAsyncRequests = 10;
        u_int16_t regroupSizeForAsyncRequests = 200;   // 300
        u_int16_t maxGroupSizeForAsyncRequests = 300;
        u_int16_t minVecSizeForGroupSplitAsyncRequests = 50;
        u_int8_t maxCallRetrys = 8;
        u_int16_t retryWaight = 800;

        std::unique_ptr<httplib::Client> AyonServer;
        const int num_threads;
        std::shared_ptr<AyonLogger> Log;
        std::regex regexVersionPattern = std::regex("/v(\\d{3})/");
        std::string uriResolverEndpoint = "/api/resolve";
        std::string uriResolverEndpointPathOnlyVar = "?pathOnly=true";
        bool pathOnlyReselution = true;

        std::mutex ConcurentRequestAfterffoMutex;
        uint8_t maxConcurentRequestAfterffo = 8;

        uint16_t GenerativeCorePostMaxLoopIterations = 200;

        u_int16_t connectionTimeOutMax = 400;
        u_int8_t readTimeOutMax = 160;
        // bool enableThreadWaithing = false;
        bool enableBigBlockThreadWaithing = true;
        bool batchResolveOptimizeVector = false;

        uint16_t ServerBusyCode = 503;
        uint16_t RequestDelayWhenServerBusy = 10000;
        std::mutex allowRequest;

        bool serverBusy = false;

        std::mutex AyonServerMutex;
};
