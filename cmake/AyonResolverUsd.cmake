include_guard(GLOBAL)

message(STATUS "[AYON] Configuring resolver dependencies for standalone USD...")

# =========================================================
# Boost include directory resolution
#
# Priority:
#   1. pxr/external/boost present in USD_INCLUDE_DIR
#        → use USD_INCLUDE_DIR as root (no extra compile def needed;
#          source code default #else branch uses <pxr/external/boost/...>)
#   2. boost/ present directly in USD_INCLUDE_DIR
#        → use USD_INCLUDE_DIR as root + set AYON_USE_BOOST
#   3. System find_package(Boost) fallback
#        → use Boost_INCLUDE_DIRS + set AYON_USE_BOOST
#
# Using USD's own include tree avoids version mismatches with the
# Boost library that USD was actually compiled against.
# =========================================================
if(EXISTS "${USD_INCLUDE_DIR}/pxr/external/boost/python.hpp")
    # Newer standalone USD bundles Boost under pxr/external.
    # The source-code default (#else) path already uses:
    #   #include <pxr/external/boost/...>
    # USD_INCLUDE_DIR is already on the include path via AyonResolverDeps,
    # so no additional include directory is required.
    set(AR_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}")
    message(STATUS "[AYON] Boost: pxr/external bundled in USD include dir (pxr_boost)")
    message(STATUS "[AYON]   AR_BOOST_INCLUDE_DIR = ${AR_BOOST_INCLUDE_DIR}")
elseif(EXISTS "${USD_INCLUDE_DIR}/boost/python.hpp")
    # Older standalone USD ships standard Boost under its include dir.
    set(AR_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}")
    list(APPEND AYON_RESOLVER_DCC_COMPILE_DEFINITIONS AYON_USE_BOOST)
    message(STATUS "[AYON] Boost: standard boost in USD include dir (AYON_USE_BOOST)")
    message(STATUS "[AYON]   AR_BOOST_INCLUDE_DIR = ${AR_BOOST_INCLUDE_DIR}")
else()
    # No Boost found inside the USD tree — fall back to a system-wide search.
    # This preserves the original behaviour for USD builds that rely on a
    # separately installed Boost.
    message(STATUS "[AYON] Boost: no bundled Boost in USD; using system find_package(Boost)")
    find_package(Boost REQUIRED)
    set(AR_BOOST_INCLUDE_DIR "${Boost_INCLUDE_DIRS}")
    list(APPEND AYON_RESOLVER_DCC_COMPILE_DEFINITIONS AYON_USE_BOOST)
endif()

if(NOT AR_BOOST_INCLUDE_DIR)
    message(FATAL_ERROR "[AYON] Could not determine Boost include directory for standalone USD")
endif()

# =========================================================
# Python resolution
#
# Priority:
#   1. Python_EXECUTABLE         — explicit user override
#   2. USD_PYTHON_EXECUTABLE     — executable from USD pxrConfig.cmake
#                                  (guarantees the same Python USD was built with)
#   3. PYTHON_EXECUTABLE         — legacy upper-case variable
#   4. Python_ROOT_DIR derived from USD_PYTHON_INCLUDE_DIR
#                                  (pxrConfig.cmake include path, no executable)
#   5. find_package(Python) system search with no extra hints
# =========================================================
if(NOT Python_FOUND)
    message(STATUS "[AYON] Python: resolving Python for standalone USD...")
    message(STATUS "[AYON]   USD_PYTHON_EXECUTABLE = ${USD_PYTHON_EXECUTABLE}")
    message(STATUS "[AYON]   USD_PYTHON_INCLUDE_DIR = ${USD_PYTHON_INCLUDE_DIR}")
    message(STATUS "[AYON]   USD_PYTHON_LIBRARY = ${USD_PYTHON_LIBRARY}")
    message(STATUS "[AYON]   Python_EXECUTABLE = ${Python_EXECUTABLE}")
    if(DEFINED Python_EXECUTABLE AND EXISTS "${Python_EXECUTABLE}")
        # Priority 1: explicit override supplied by the caller
        message(STATUS "[AYON] Python: using caller-provided Python_EXECUTABLE")
        get_filename_component(_py_root "${Python_EXECUTABLE}" DIRECTORY)
        get_filename_component(_py_root "${_py_root}" DIRECTORY)
        set(Python_ROOT_DIR "${_py_root}")

    elseif(DEFINED USD_PYTHON_EXECUTABLE AND EXISTS "${USD_PYTHON_EXECUTABLE}")
        # Priority 2: executable recorded in the USD build's pxrConfig.cmake
        message(STATUS "[AYON] Python: using USD pxr config executable: ${USD_PYTHON_EXECUTABLE}")
        set(Python_EXECUTABLE "${USD_PYTHON_EXECUTABLE}")
        get_filename_component(_py_root "${Python_EXECUTABLE}" DIRECTORY)
        get_filename_component(_py_root "${_py_root}" DIRECTORY)
        set(Python_ROOT_DIR "${_py_root}")
        # Forward exact library/include hints so FindPython selects the same installation
        if(DEFINED USD_PYTHON_LIBRARY AND EXISTS "${USD_PYTHON_LIBRARY}")
            set(Python_LIBRARY "${USD_PYTHON_LIBRARY}")
        endif()
        if(DEFINED USD_PYTHON_INCLUDE_DIR AND EXISTS "${USD_PYTHON_INCLUDE_DIR}")
            set(Python_INCLUDE_DIR "${USD_PYTHON_INCLUDE_DIR}")
        endif()

    elseif(DEFINED PYTHON_EXECUTABLE AND EXISTS "${PYTHON_EXECUTABLE}")
        # Priority 3: legacy upper-case variable
        message(STATUS "[AYON] Python: using legacy PYTHON_EXECUTABLE: ${PYTHON_EXECUTABLE}")
        set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
        get_filename_component(_py_root "${Python_EXECUTABLE}" DIRECTORY)
        get_filename_component(_py_root "${_py_root}" DIRECTORY)
        set(Python_ROOT_DIR "${_py_root}")

    elseif(DEFINED USD_PYTHON_INCLUDE_DIR AND EXISTS "${USD_PYTHON_INCLUDE_DIR}")
        # Priority 4: no executable available, but pxrConfig.cmake recorded the
        # include dir — derive Python_ROOT_DIR from it and hint the library.
        message(STATUS "[AYON] Python: deriving root from USD pxr config include dir: ${USD_PYTHON_INCLUDE_DIR}")
        get_filename_component(_py_root "${USD_PYTHON_INCLUDE_DIR}" DIRECTORY)
        set(Python_ROOT_DIR "${_py_root}")
        if(DEFINED USD_PYTHON_LIBRARY AND EXISTS "${USD_PYTHON_LIBRARY}")
            set(Python_LIBRARY "${USD_PYTHON_LIBRARY}")
        endif()
    endif()

    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    unset(_py_root)
endif()

# =========================================================
# USD Python library — provides pxr_boost::python symbols
#
# usd_python.dll (/ usd_python.lib on Windows) re-exports the
# pxr_boost::python runtime (registry, converters, …) that the
# Python wrapping layer calls directly.  Without this the linker
# emits LNK2019 for every pxr_boost::python::converter::* symbol.
# pxr_boost builds shipped in USD < 0.22 used "pxr_python" instead.
# =========================================================
find_library(_usd_standalone_python_lib
    NAMES usd_python pxr_python
    PATHS "${USD_LIB_DIR}"
    NO_DEFAULT_PATH
)
if(_usd_standalone_python_lib)
    set(USD_PYTHON_LIBRARIES "${_usd_standalone_python_lib}")
    list(APPEND AYON_RESOLVER_DCC_LINK_LIBS "${USD_PYTHON_LIBRARIES}")
    message(STATUS "[AYON] USD Python lib: ${USD_PYTHON_LIBRARIES}")
else()
    message(WARNING
        "[AYON] Could not find usd_python or pxr_python in ${USD_LIB_DIR}. "
        "Linking may fail with unresolved pxr_boost::python symbols.")
endif()
unset(_usd_standalone_python_lib CACHE)

# =========================================================
# Propagate include directories to AYON_RESOLVER_DCC_INCLUDE_DIRS
# =========================================================
list(APPEND AYON_RESOLVER_DCC_INCLUDE_DIRS
    "${AR_BOOST_INCLUDE_DIR}"
    "${Python_INCLUDE_DIRS}"
)
