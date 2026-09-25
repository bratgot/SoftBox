SoftBox - soft shadow area light for Nuke 17 ScanlineRender2
By Marten Blumen - MIT license - https://github.com/bratgot/SoftBox

Adds an AreaLight node (3D > Lights > AreaLight) with real soft shadows.
Nuke's own lights send one shadow ray to the light's centre, so their
shadows are hard. SoftBox's slrRectLight shader sends shadow rays to
random points across the whole light surface, like a path tracer, so
ScanlineRender2 renders a smooth, physically based penumbra from a single
light. The v1.0 grid-of-lights method is still there as Grid mode.

Supported: Nuke 17.0 and Nuke 17.1, Windows x64.


INSTALL
-------

1. Copy this whole SoftBox folder into your .nuke folder:

       C:\Users\<you>\.nuke\SoftBox\

2. Add this line to C:\Users\<you>\.nuke\init.py (create the file if it
   does not exist):

       nuke.pluginAddPath('./SoftBox')

3. Restart Nuke. AreaLight appears under 3D > Lights.

SoftBox picks the build that matches your Nuke version (17.0/ or 17.1/)
automatically. If you had an older AreaLight.dll directly in .nuke, delete
it - Nuke would try to load it in every version.


USAGE
-----

1. Create an AreaLight node and connect it to your scene.
2. Set Width and Height for the light surface size.
3. Adjust Intensity, Exposure and Color.
4. On the Shadow tab, leave Shadow Mode on Stochastic and set Shadow
   Samples for smoothness:

       1   noisy - relies on ScanlineRender2 camera samples
       16  smooth with light grain - previews
       64  clean - final renders, close-ups

5. Connect to ScanlineRender2 and render.

Shadow softness comes from the light's size (Width x Height); Shadow Samples
only controls how smooth the penumbra gradient is.

Scripts saved with v1.0/v1.1 open in Stochastic mode. Set Shadow Mode to
Grid for the old look (the two modes differ slightly in brightness).

Both AreaLight.dll and slrRectLight.dll must stay together in the version
folder (17.0/ or 17.1/).
