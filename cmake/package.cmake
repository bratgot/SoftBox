# Stages a ready-to-upload distribution from every configured build directory.
#
#   cmake -P cmake/package.cmake
#
# Looks for build*/ folders (one per Nuke minor version, each already built
# with --config Release), installs their "plugin" component into
#   dist/SoftBox-v<ver>/SoftBox/
# and zips that as dist/SoftBox-v<ver>-Nuke17-win64.zip. The zip holds a single
# SoftBox/ folder, which users drop into ~/.nuke.
cmake_minimum_required(VERSION 3.20)

get_filename_component(_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

file(READ "${_root}/CMakeLists.txt" _lists)
string(REGEX MATCH "project\\(AreaLight VERSION ([0-9]+\\.[0-9]+)" _m "${_lists}")
set(_ver "${CMAKE_MATCH_1}")
if(NOT _ver)
    message(FATAL_ERROR "Could not read the project VERSION from CMakeLists.txt")
endif()

set(_name  "SoftBox-v${_ver}")
set(_stage "${_root}/dist/${_name}")
set(_zip   "${_root}/dist/${_name}-Nuke17-win64.zip")

file(REMOVE_RECURSE "${_stage}")
file(REMOVE "${_zip}")

file(GLOB _dirs LIST_DIRECTORIES true "${_root}/build*")
set(_count 0)
foreach(_d ${_dirs})
    if(NOT EXISTS "${_d}/CMakeCache.txt" OR NOT EXISTS "${_d}/plugin/Release/AreaLight.dll")
        continue()
    endif()
    message(STATUS "Staging ${_d}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --install "${_d}" --config Release
                --component plugin --prefix "${_stage}"
        RESULT_VARIABLE _rc)
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "Install of ${_d} failed")
    endif()
    math(EXPR _count "${_count} + 1")
endforeach()

if(_count EQUAL 0)
    message(FATAL_ERROR "No built build*/ directories found. Build first.")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${_zip}" --format=zip SoftBox
    WORKING_DIRECTORY "${_stage}"
    RESULT_VARIABLE _rc)
if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "Zipping failed")
endif()

message(STATUS "Packaged ${_count} Nuke build(s): ${_zip}")
