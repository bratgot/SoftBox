# SoftBox init.py - runs in every Nuke mode (GUI, -t, -ti, farm).
# The compiled plugin is tied to one Nuke minor version, so binaries live in
# per-version subfolders (17.0/, 17.1/, ...). Add the one matching this Nuke.
import os

import nuke

_dir = os.path.dirname(os.path.abspath(__file__))
_mm = "%d.%d" % (nuke.NUKE_VERSION_MAJOR, nuke.NUKE_VERSION_MINOR)
_bin = os.path.join(_dir, _mm)
if os.path.isdir(_bin):
    nuke.pluginAddPath(_bin.replace("\\", "/"), addToSysPath=False)
elif nuke.NUKE_VERSION_MAJOR >= 17:
    nuke.tprint("SoftBox: no AreaLight build for Nuke %s (looked in %s)" % (_mm, _bin))
