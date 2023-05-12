#pragma once

#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/json.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "AYON_AssetResolvingProvider.h"

class AYON_AssetResolveResult;


class AYON_AssetResolvingProvider_REST : public AYON_AssetResolvingProvider {

public:
    //Version of HTTP protocol, value 11 equals to 1.1
    int Version = 11;
    //Enable/Disable dumping of info log
    bool bEnableDebugLog = true;
    //Enable/Disable dumping of error log
    bool bEnableErrorLog = true;
    //DNS name
    std::string Host = "localhost";
    //Port where is listening server
    int Port = 5000;


    //Path where is endpoint on webserver
    std::string UsdApiEndpoint = "/api/usd";

    /// <summary>
    /// Inside this method is written main logic for calling REST API.
    /// </summary>
    /// <param name="AssetPath">Unresolved path</param>
    /// <returns>Result object with resolved path when everything is ok, in case of problems there should be filled error message too.</returns>
    AYON_AssetResolveResult Resolve(const std::string& AssetPath) override;

private:
    /// <summary>
    /// This method creates boost request add fills header "unresolved_path" with value of parameter @AssetPath
    /// </summary>
    /// <param name="AssetPath">Unresolved path</param>
    /// <returns>Boost request object</returns>
    boost::beast::http::request<boost::beast::http::string_body> Request_Create(const std::string& AssetPath);

    /// <summary>
    /// This method parses value for key "resolved_path" from response.
    /// </summary>
    /// <param name="Response">Object of response</param>
    /// <returns>Value of resolved_path</returns>
    std::string Response_Parse(const boost::beast::http::response<boost::beast::http::dynamic_body>& Response, const std::string& AssetPath);
};


