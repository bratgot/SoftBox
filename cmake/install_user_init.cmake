# Appends the SoftBox registration block to <prefix>/init.py once.
# Detects the marker on later runs and leaves the file alone; never rewrites
# the user's existing content.
set(_init "${CMAKE_INSTALL_PREFIX}/init.py")
set(_marker "# --- SoftBox (auto-added by cmake --install) ---")

set(_existing "")
if(EXISTS "${_init}")
    file(READ "${_init}" _existing)
endif()

string(FIND "${_existing}" "${_marker}" _pos)
if(_pos EQUAL -1)
    set(_block "${_marker}\nimport nuke\nnuke.pluginAddPath('./SoftBox')\n# --- end SoftBox ---\n")
    if(_existing AND NOT _existing MATCHES "\n$")
        set(_block "\n${_block}")
    endif()
    file(APPEND "${_init}" "${_block}")
    message(STATUS "SoftBox: registered in ${_init}")
else()
    message(STATUS "SoftBox: already registered in ${_init}")
endif()
