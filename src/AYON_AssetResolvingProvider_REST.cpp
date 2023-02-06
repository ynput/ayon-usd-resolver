#include "AYON_AssetResolvingProvider_REST.h"
#include <boost/exception/diagnostic_information.hpp> 
#include <iostream>
#include "Logging/AYON_Logging.h"


AYON_AssetResolveResult AYON_AssetResolvingProvider_REST::Resolve(const std::string& AssetPath) {

    AYON_AssetResolveResult Result;
    try 
    {
        //TCP
        //-------------------
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::resolver resolver(ioc);
        boost::beast::tcp_stream stream(ioc);
        //convert DNS name to ip address and port
        auto const results = resolver.resolve(Host, std::to_string(Port));
        stream.connect(results);

        // HTTP Request
        //-------------------
        auto req = Request_Create(AssetPath);
        boost::beast::http::write(stream, req);

        //HTTP Response Reading
        //-------------------
        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::dynamic_body> res;
        boost::beast::http::read(stream, buffer, res);
        AYON_LOG_INFO("Response", res);

        // Gracefully close the socket
        boost::beast::error_code ec;
        stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        //Parsing JSON
        //-------------------
        Result.ResolvedPath = Response_Parse(res)+".7";
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

boost::beast::http::request<boost::beast::http::string_body> AYON_AssetResolvingProvider_REST::Request_Create(const std::string& AssetPath)
{
    boost::beast::http::request<boost::beast::http::string_body> req{ boost::beast::http::verb::get, ResolvingUrlEndpointPath, Version };
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set("unresolved_path", AssetPath);
    AYON_LOG_INFO("Request", req);

    return req;
}

std::string AYON_AssetResolvingProvider_REST::Response_Parse(const boost::beast::http::response<boost::beast::http::dynamic_body>& Response)
{
    auto pt = GetPropertyTreeFromResponse(Response);
    auto resolved_path = pt.get<std::string>("resolved_path");
    AYON_LOG_INFO("Resolved Path", resolved_path);

    return resolved_path;
}

boost::property_tree::ptree AYON_AssetResolvingProvider_REST::GetPropertyTreeFromResponse(const boost::beast::http::response<boost::beast::http::dynamic_body>& Response)
{
    auto body_str = boost::beast::buffers_to_string(Response.body().data());
    AYON_LOG_INFO("Response body", body_str);

    std::stringstream ss;
    ss << body_str;
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);

    return pt;
}

