project(YON_integration_for_USD)
cmake_minimum_required(VERSION 3.13.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Setup VS (tested 2019):
if(MSVC)
    # C/C++ >>> Language >>> Remove unreferenced code and data = No
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:inline-")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DHAVE_SNPRINTF")
endif()

# Root where will be placed output for installation
set(CMAKE_INSTALL_PREFIX "." CACHE PATH "..." FORCE)

#WINDOWS
if(MSVC)
    message(">>> Windows")
    #NOT WINDOWS
    else()
        #UNIX
        if(UNIX)  
            message(">>> Unix")          
            if(NOT APPLE)
                set(LINUX TRUE)
            endif()            
        endif()

        #LINUX
        if(LINUX) 
            message(">>> Linux")
        endif()
endif()

find_package(Boost REQUIRED)
find_package(PythonLibs)

# USD environment variable and variables extracted from the value
message("Environment variable USD = $ENV{USD}")
set(USD_GENSCHEMA "$ENV{USD}/bin/usdGenSchema")
set(USD_INCLUDE_DIR "$ENV{USD}/include")
set(USD_LIBRARY_DIR "$ENV{USD}/lib")

add_subdirectory(src)