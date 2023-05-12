#include "AYON_AssetResolvingProvider_REST.h"
#include <boost/exception/diagnostic_information.hpp> 
#include <iostream>
#include "Logging/AYON_Logging.h"

namespace Asio = boost::asio;
namespace Beast = boost::beast;
namespace Http = boost::beast::http;


AYON_AssetResolveResult AYON_AssetResolvingProvider_REST::Resolve(const std::string& AssetPath) {

    AYON_AssetResolveResult Result;
    try 
    {
        // Set up the TCP 
        Asio::io_context ioc;
        Asio::ip::tcp::resolver resolver(ioc);
        Beast::tcp_stream stream(ioc);
        Host = "localhost";
        Port = 5000;

        // Set up the API request based on the ENV VAR
        char * ayon_server_env_var = getenv("AYON_SERVER_URL");
        std::string ayon_server = ayon_server_env_var == NULL ? std::string("") : std::string(ayon_server_env_var);
        
        if (!ayon_server.empty()) {
            size_t colon_char_index = ayon_server.find_first_of(":");

            if (colon_char_index) {
                // Split URL and Port
                Host = ayon_server.substr(0, colon_char_index);
                Port = std::stoi(ayon_server.substr(colon_char_index + 1));
            } else {
                // Use the provided URL with default PORT (5000)
                Host = ayon_server;
                //Port where is listening server
                Port = 5000;
            }
        }
        

        std::cout << "Host: " + Host << std::endl;
        std::cout << "Port: " + Port << std::endl;

        //convert DNS name to ip address and port
        auto const results = resolver.resolve(Host, std::to_string(Port));
        stream.connect(results);

        // HTTP Request
        //-------------------
        auto req = Request_Create(AssetPath);
        Http::write(stream, req);

        //HTTP Response Reading
        //-------------------
        Beast::flat_buffer buffer;
        Http::response<Http::dynamic_body> res;
        Http::read(stream, buffer, res);
        AYON_LOG_INFO("Response", res);

        // Gracefully close the socket
        Beast::error_code ec;
        stream.socket().shutdown(Asio::ip::tcp::socket::shutdown_both, ec);

        //Parsing JSON
        //-------------------
        
        Result.ResolvedPath = Response_Parse(res, AssetPath);
        Result.bResolved = true;
    }
    catch (const boost::exception& Ex) 
    {
        Result.ErrorMsg = AYON_LOG_EXCEPTION("boost exception", Ex, boost::current_exception_diagnostic_information());
    }
    catch (std::exception const& Ex) 
    {
        Result.ErrorMsg = AYON_LOG_EXCEPTION("std::exception", Ex, Ex.what());
    }

    return Result;
}

Http::request<Http::string_body> AYON_AssetResolvingProvider_REST::Request_Create(const std::string& AssetPath)
{
    Http::request<Http::string_body> req {
        Http::verb::post,
        UsdApiEndpoint,
        Version
    };


    req.set(Http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(Http::field::content_type, "application/json");

    char * ayon_api_key_env_var = getenv("AYON_API_KEY");
    std::string ayon_api_key = ayon_api_key_env_var == NULL ? std::string("") : std::string(ayon_api_key_env_var);

    if (!ayon_api_key.empty()) {
        req.set("X-Api-Key", ayon_api_key);
    }

    req.body() = "{\"uris\": [\"" + AssetPath + "\"]}";
    req.prepare_payload();
    AYON_LOG_INFO("Sending request", req);

    return req;
}

std::string AYON_AssetResolvingProvider_REST::Response_Parse(const Http::response<Http::dynamic_body>& Response, const std::string& AssetPath)
{
    std::stringstream response_ss;
    response_ss << Beast::buffers_to_string(Response.body().data());

    boost::property_tree::ptree response_ptree;
    boost::property_tree::read_json(response_ss, response_ptree);

    /* Explaination of how the JSON maps to a ptree, and how to traverse the
    list in the "paths" key: https://stackoverflow.com/a/59227626 */
    auto paths = response_ptree.get_child("paths");

    for (auto path_uri = paths.begin(); path_uri != paths.end(); ++path_uri) {
        for (auto path_resolve = path_uri->second.begin(); path_resolve != path_uri->second.end(); ++path_resolve) {
            if (path_resolve->first == AssetPath) {
                AYON_LOG_INFO("Resolved Path", path_resolve->second.data());
                return path_resolve->second.data();
            }
        }
    }

    return "";
}


