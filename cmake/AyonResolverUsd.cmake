include_guard(GLOBAL)

message(STATUS "[AYON] Configuring resolver dependencies for standalone USD...")

find_package(Boost REQUIRED)
set(AR_BOOST_INCLUDE_DIR "${Boost_INCLUDE_DIRS}")

if(NOT Python_FOUND)
    if(DEFINED Python_EXECUTABLE)
        set(_active_py_exec "${Python_EXECUTABLE}")
    elseif(DEFINED PYTHON_EXECUTABLE)
        set(_active_py_exec "${PYTHON_EXECUTABLE}")
        set(Python_EXECUTABLE "${_active_py_exec}")
    endif()

    if(DEFINED _active_py_exec)
        get_filename_component(_python_root "${_active_py_exec}" DIRECTORY)
        get_filename_component(_python_root "${_python_root}" DIRECTORY)
        set(Python_ROOT_DIR "${_python_root}")
    endif()

    find_package(Python COMPONENTS Interpreter Development REQUIRED)
endif()

list(APPEND AYON_RESOLVER_DCC_INCLUDE_DIRS
    "${Boost_INCLUDE_DIRS}"
    "${Python_INCLUDE_DIRS}"
)
