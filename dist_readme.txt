SoftBox - soft shadow area light for Nuke 17 ScanlineRender2
By Marten Blumen - MIT license - https://github.com/bratgot/SoftBox

Adds an AreaLight node (3D > Lights > AreaLight) that emits a grid of
sub-lights across a rectangular surface, giving ScanlineRender2 smooth,
physically plausible penumbras instead of hard shadows.

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
4. Set Shadow Samples to control shadow smoothness:

       1   hard shadow      look-dev, fast iteration
       4   2x2 grid         previz
       16  4x4 grid         recommended default
       64  8x8 grid         final renders, close-ups

5. Connect to ScanlineRender2 and render.

Shadow softness comes from the light's size (Width x Height); Shadow Samples
only controls how smooth the penumbra gradient is. Render time scales
linearly with the sample count.
