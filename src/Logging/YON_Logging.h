#pragma once
#include <string>
#include <iostream>
#include <sstream>

constexpr bool LOG_INFO = true;
constexpr bool LOG_EXCEPTION = true;
const std::string SEPARATOR = "==============================================================================";
const std::string SEPARATOR_INTERNAL = "---------------------------------------------------------------------";

#define YON_LOG_EXCEPTION(Realm, Ex, Message)\
    YON_LogUtils::Log_Exception(Realm, Ex, Message)

#define YON_LOG_INFO(Category, Message)\
    YON_LogUtils::Log_Info(Category, Message)

class YON_LogUtils 
{
public:
	template<typename ExceptionInstance, typename ExceptionMessage>
	static std::string Log_Exception(const std::string& Realm, const ExceptionInstance& Ex, const ExceptionMessage& Message);

    template<typename T>
    static void Log_Info(const std::string& Category, const T& Message);

private:
    static void WriteToStream(const std::string& Caption, const std::string& Body);
};

template<typename ExceptionInstance, typename ExceptionMessage>
inline std::string YON_LogUtils::Log_Exception(const std::string& Realm, const ExceptionInstance& Ex, const ExceptionMessage& Message)
{
    if (!LOG_EXCEPTION) {
        std::ostringstream StringStream;
        StringStream << Message;
        std::string MessageStr = StringStream.str();
        return MessageStr;
    }

    auto TypeId = typeid(Ex).name();

    std::stringstream Body;
    Body << TypeId << std::endl
         << SEPARATOR_INTERNAL << std::endl
         << Message;

    const std::string REALM_LINE_FIX = " !!!!!!!!!!!!!!!!!!! ";
    WriteToStream(REALM_LINE_FIX +" Error/Exception: " + Realm + REALM_LINE_FIX, Body.str());

    return Message;
}

template<typename T>
inline void YON_LogUtils::Log_Info(const std::string& Caption, const T& Message)
{
    if (!LOG_INFO)
        return;

    std::stringstream Body;
    Body << Message;

    WriteToStream(Caption, Body.str());
}