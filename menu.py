# SoftBox menu.py - adds AreaLight to the toolbar under 3D > Lights.
# Installed as ~/.nuke/SoftBox/menu.py; only registers when init.py found a
# build for this Nuke version.
import os

import nuke

_mm = "%d.%d" % (nuke.NUKE_VERSION_MAJOR, nuke.NUKE_VERSION_MINOR)
if os.path.isdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), _mm)):
    toolbar = nuke.toolbar("Nodes")
    lightMenu = toolbar.addMenu("3D/Lights", icon="Light.png")
    lightMenu.addCommand("AreaLight", "nuke.createNode('AreaLight')", icon="Light.png")
