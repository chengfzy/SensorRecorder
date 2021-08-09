# Get the commit hash and date of current source using git
macro(GetGitInfo)
    find_package(Git QUIET)
    if (GIT_FOUND)
        # hash
        execute_process(COMMAND ${GIT_EXECUTABLE} log -n 1 --pretty=format:%h
            OUTPUT_VARIABLE GitHash
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
        # date
        execute_process(COMMAND ${GIT_EXECUTABLE} log -n 1 --pretty=format:%cI
            OUTPUT_VARIABLE GitDate
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
        # remove last +0800
        string(SUBSTRING "${GitDate}" 0 19 GitDate)
        message(STATUS "Git hash: ${GitHash}, date: ${GitDate}")
    endif ()
endmacro ()

# Check the compiler of windows
macro(CheckWinCompiler)
    message(STATUS "Check VC compiler for Windows")
    if (WIN32)
        # check VS version
        if (${MSVC_TOOLSET_VERSION} EQUAL 140)
            message(STATUS "Using VC++ 2015")
            set(VCVersion "VC2015")
        elseif (${MSVC_TOOLSET_VERSION} EQUAL 141)
            message(STATUS "Using VC++ 2017")
            set(VCVersion "VC2017")
        elseif (${MSVC_TOOLSET_VERSION} EQUAL 142)
            message(STATUS "Using VC++ 2019")
            set(VCVersion "VC2019")
        else ()
            message(WARNING "  Unsupported MSVC version: ${MSVC_TOOLSET_VERSION}")
        endif ()
        # check windows arch, and set the suffix
        if (${CMAKE_GENERATOR} MATCHES "(Win64|IA64)")
            message(STATUS "VC++ Arch: x64")
            set(VCArch "x64")
        elseif (${CMAKE_SIZEOF_VOID_P} STREQUAL "8")
            message(STATUS "VC++ Arch: x64")
            set(VCArch "x64")
        else ()
            message(STATUS "VC++ Arch: x86")
            set(VCArch "x86")
        endif ()
    endif ()
endmacro ()


# find Qt folder and add it to cmake search path, cannot use function() due to CMAKE_PREFIX_PATH will be a local variable
macro(FindAddQtPath QtPaths)
    # check windows arch and VC version
    if (WIN32)
        CheckWinCompiler()
        # VC suffix
        if (${VCVersion} STREQUAL "VC2015")
            set(_QtVcSuffix "msvc2015")
        elseif (${VCVersion} STREQUAL "VC2017")
            set(_QtVcSuffix "msvc2017")
        elseif (${VCVersion} STREQUAL "VC2019")
            set(_QtVcSuffix "msvc2017")
        endif ()
        # arch suffix
        if (${VCArch} STREQUAL "x64")
            set(_QtArchSuffix "_64")
        elseif (${VCArch} STREQUAL "x86")
            set(_QtArchSuffix "")
        endif ()                
    endif ()

    # check input candidate folder and add it to cmake search path
    foreach (r ${ARGV})
        if (${r} MATCHES "cmake")
            message(STATUS "Add Qt path: ${r}")
            list(APPEND CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${r}")
        else ()
            # find Qt folder
            file(GLOB FOLDER RELATIVE ${r} ${r}/*)
            foreach (f ${FOLDER})
                if (IS_DIRECTORY ${r}/${f} AND ${f} MATCHES "Qt[0-9.]+")
                    # parse Qt version and set Qt path
                    string(SUBSTRING ${f} 2 -1 _QtVersion)
                    if (UNIX)
                        set(_QtCMakePath "${r}/${f}/${_QtVersion}/gcc_64/lib/cmake")
                    elseif (WIN32)
                        set(_QtCMakePath "${r}/${f}/${_QtVersion}/${_QtVcSuffix}${_QtArchSuffix}")
                    endif ()
                    # check path exist and add to CMake path
                    if (IS_DIRECTORY ${_QtCMakePath})
                        message(STATUS "Found Qt ${_QtVersion} in ${r}/${f}")
                        message(STATUS "Add Qt path: ${_QtCMakePath}")
                        list(APPEND CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${_QtCMakePath}")
                        break ()
                    endif ()
                endif ()
            endforeach ()
        endif ()
    endforeach ()
endmacro ()