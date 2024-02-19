#ifndef APPDATA_UTILS_H
#define APPDATA_UTILS_H

#include <string>
#include <cstdlib>
#include <filesystem>
#include "AyonCppApiCrossPlatformMacros.h"

#ifdef _WIN32
    #include <windows.h>
    #define OS_WINDOWS
#elif __APPLE__
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
    #define OS_MACOS
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
    #define OS_LINUX
#endif

inline std::string
getAppDataDir() {
    std::string appDataDir;

#ifdef OS_WINDOWS
    const wchar_t* appDataPath = _wgetenv(L"APPDATA");
    if (appDataPath) {
        std::filesystem::path winPath = std::filesystem::path(appDataPath);
        appDataDir = winPath.string();
}
#elif defined(OS_MACOS)
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        appDataDir = std::string(homeDir) + "/Library/Application Support";
    }
#else   // Linux and other Unix-like systems
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        appDataDir = std::string(homeDir) + "/.local/share";
    }
#endif

    return appDataDir;
}

#endif   // APPDATA_UTILS_H
