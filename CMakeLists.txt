cmake_minimum_required(VERSION 3.20)
project(AreaLight VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Locate Nuke 17 --------------------------------------------------------
set(NUKE_DIR "" CACHE PATH "Root of the Nuke 17 installation")

if(NOT NUKE_DIR)
    # Newest first so the latest installed Nuke 17 wins
    foreach(_ver "17.1v1" "17.0v4" "17.0v3" "17.0v2" "17.0v1" "17.0v1-Beta.5")
        if(EXISTS "C:/Program Files/Nuke${_ver}/include/DDImage/GeomOp.h")
            set(NUKE_DIR "C:/Program Files/Nuke${_ver}")
            break()
        endif()
    endforeach()
endif()

if(NOT NUKE_DIR)
    message(FATAL_ERROR
        "Cannot find Nuke 17. Set -DNUKE_DIR=<path> to your Nuke install directory.\n"
        "Example: cmake -S . -B build --preset vs2022 -DNUKE_DIR=\"C:/Program Files/Nuke17.0v1\"")
endif()

set(NUKE_INCLUDE "${NUKE_DIR}/include")

# The DDImage ABI is pinned per minor release, so the build is tagged with the
# NDK's own major.minor and installed into a matching subfolder.
file(STRINGS "${NUKE_INCLUDE}/DDImage/ddImageVersionNumbers.h" _nuke_ver_lines
     REGEX "#define kDDImageVersion(Major|Minor)Num ")
string(REGEX REPLACE ".*MajorNum ([0-9]+).*" "\\1" NUKE_VERSION_MAJOR "${_nuke_ver_lines}")
string(REGEX REPLACE ".*MinorNum ([0-9]+).*" "\\1" NUKE_VERSION_MINOR "${_nuke_ver_lines}")
set(NUKE_VERSION_MM "${NUKE_VERSION_MAJOR}.${NUKE_VERSION_MINOR}")

message(STATUS "Nuke directory: ${NUKE_DIR} (NDK ${NUKE_VERSION_MM})")

# ---- Required libraries -----------------------------------------------------
set(NUKE_LIBS
    "${NUKE_DIR}/DDImage.lib"
    "${NUKE_DIR}/FnUsdAbstraction.lib"
    "${NUKE_DIR}/FnUsdEngine.lib"
    "${NUKE_DIR}/FdkBase.2.1.lib"
)

foreach(_lib ${NUKE_LIBS})
    if(NOT EXISTS "${_lib}")
        message(FATAL_ERROR "Required library not found: ${_lib}")
    endif()
endforeach()

# ---- Plugin ----------------------------------------------------------------
add_library(AreaLight MODULE
    UsdAreaLightOp.cpp
    UsdAreaLightOp.h
)

target_include_directories(AreaLight PRIVATE "${NUKE_INCLUDE}")
target_link_libraries(AreaLight PRIVATE ${NUKE_LIBS} opengl32)

target_compile_definitions(AreaLight PRIVATE
    FN_OS_WINDOWS
    NOMINMAX
    _USE_MATH_DEFINES
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
)

set_target_properties(AreaLight PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugin"
)

# ---- Install ----------------------------------------------------------------
# Layout (one folder per plugin, one subfolder per Nuke minor version):
#   <prefix>/SoftBox/init.py, menu.py, LICENSE
#   <prefix>/SoftBox/<major.minor>/AreaLight.dll
# plus an idempotent nuke.pluginAddPath('./SoftBox') block in <prefix>/init.py.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(WIN32)
        set(CMAKE_INSTALL_PREFIX "$ENV{USERPROFILE}/.nuke" CACHE PATH "Nuke user directory" FORCE)
    else()
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.nuke"        CACHE PATH "Nuke user directory" FORCE)
    endif()
endif()

# Component "plugin" is the SoftBox folder itself (also what cmake/package.cmake
# stages for distribution); "register" edits the user's own init.py.
install(TARGETS AreaLight
        RUNTIME DESTINATION "SoftBox/${NUKE_VERSION_MM}" COMPONENT plugin
        LIBRARY DESTINATION "SoftBox/${NUKE_VERSION_MM}" COMPONENT plugin)

install(FILES init.py menu.py LICENSE DESTINATION SoftBox COMPONENT plugin)
install(FILES dist_readme.txt DESTINATION SoftBox RENAME README.txt COMPONENT plugin)

install(SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_user_init.cmake"
        COMPONENT register)
