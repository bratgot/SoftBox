# SoftBox

**Soft shadow area light for Nuke 17 ScanlineRender2**

By Marten Blumen

![Nuke 17](https://img.shields.io/badge/Nuke-17.0%20%7C%2017.1-yellow) ![License](https://img.shields.io/badge/license-MIT-blue) ![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)

---

ScanlineRender2's built-in lights only cast hard shadows. SoftBox produces smooth, physically plausible penumbras by emitting a grid of sub-lights spread across a rectangular light surface — the same technique used in production VFX before stochastic shadow sampling became standard.

## Features

- **Soft shadows** via multi-light grid (1 to 64+ sub-lights)
- **Artist-friendly** — single Shadow Samples knob controls quality vs speed
- **Energy conserving** — total illumination stays constant regardless of sample count
- **Full transform** — translate, rotate, scale via standard Axis_knob
- **Viewport wireframe** — orange rectangle with direction arrow
- **USD native** — creates standard DiskLightPrims on the scene graph

## Installation

### From Release

1. Download `SoftBox-v1.1-Nuke17-win64.zip` from [Releases](https://github.com/bratgot/SoftBox/releases/latest)
2. Unzip it into your `.nuke` folder so you get `C:\Users\<you>\.nuke\SoftBox\`
3. Add this line to `~/.nuke/init.py` (create the file if needed):
   ```python
   nuke.pluginAddPath('./SoftBox')
   ```
4. Restart Nuke 17.0 or 17.1 and find **AreaLight** under **3D > Lights**

The plugin is tied to one Nuke minor version (the DDImage ABI changes between
17.0 and 17.1), so the zip carries a build for each, and SoftBox loads the one
matching the running Nuke:

```
~/.nuke/
|-- init.py                  # nuke.pluginAddPath('./SoftBox')
`-- SoftBox/
    |-- init.py              # adds the <major.minor>/ folder matching the running Nuke
    |-- menu.py              # 3D > Lights > AreaLight
    |-- README.txt
    |-- LICENSE
    |-- 17.0/AreaLight.dll
    `-- 17.1/AreaLight.dll
```

Upgrading from v1.0? Delete the old `~/.nuke/AreaLight.dll` and the
AreaLight lines in `~/.nuke/menu.py`. A DLL loose in `~/.nuke/` is loaded by
every Nuke version.

### Build From Source

Requires the **Visual Studio 2019** toolset (v142, the toolchain Foundry builds
Nuke 14.1-17.1 with) and **CMake 3.20+**. Point `NUKE_DIR` at the Nuke you are
building for; the install subfolder is taken from that NDK's version.

```bash
git clone https://github.com/bratgot/SoftBox.git
cd SoftBox

# Nuke 17.1
cmake -S . -B build --preset vs2019 -DNUKE_DIR="C:/Program Files/Nuke17.1v1"
cmake --build build --config Release
cmake --install build --config Release

# Nuke 17.0 (separate build directory)
cmake -S . -B build-17.0 -G "Visual Studio 16 2019" -A x64 -DNUKE_DIR="C:/Program Files/Nuke17.0v4"
cmake --build build-17.0 --config Release
cmake --install build-17.0 --config Release
```

The DLL is built to `build/plugin/Release/AreaLight.dll`. `cmake --install`
installs to `~/.nuke` by default (override with `--prefix`) and adds the
registration block to `~/.nuke/init.py` only if it is not already there.

If a build directory was configured before this layout existed, reconfigure it
with `cmake --fresh ...` so the install prefix is reset.

### Packaging a Release

After building every Nuke version you want to ship:

```bash
cmake -P cmake/package.cmake
```

This stages `dist/SoftBox-v<version>/SoftBox/` from every `build*/` directory
and zips it as `dist/SoftBox-v<version>-Nuke17-win64.zip`, ready for GitHub
Releases or Nukepedia. The version comes from `project(... VERSION ...)` in
`CMakeLists.txt`. `dist/` is git-ignored.

## Usage

1. Create an **AreaLight** node and connect it to your scene
2. Set **Width** and **Height** for the light surface size
3. Adjust **Intensity**, **Exposure**, and **Color**
4. Set **Shadow Samples** to control shadow softness:

| Shadow Samples | Grid | Quality | Use Case |
|---|---|---|---|
| 1 | — | Hard shadow | Look-dev, fast iteration |
| 4 | 2×2 | Basic penumbra | Previz |
| 16 | 4×4 | Smooth | Recommended default |
| 64 | 8×8 | Very smooth | Final renders, close-ups |

5. Connect to **ScanlineRender2** and render

## How It Works

ScanlineRender2's DiskLight shader shoots a single shadow ray to the light centre, producing a hard edge. SoftBox creates N DiskLightPrims arranged in a grid across the rectangular surface. Each sub-light gets `intensity / N` so total illumination is conserved. The overlapping hard shadows from different positions blend into a smooth penumbra gradient.

Shadow softness is determined by the light's physical size (Width × Height). Shadow Samples controls the smoothness of the gradient, not the penumbra width.

## Tips

- Shadow softness = light size. Bigger light = wider penumbra
- Shadow Samples = smoothness. More samples = less banding
- Keep at 1 during look-dev, crank up for finals
- Render time scales linearly with sample count

## Project Structure

```
SoftBox/
├── UsdAreaLightOp.cpp    # Plugin implementation
├── UsdAreaLightOp.h      # Plugin header
├── CMakeLists.txt        # Build configuration
├── CMakePresets.json     # VS2019 / VS2022 presets
├── cmake/
│   ├── install_user_init.cmake  # one-time ~/.nuke/init.py registration
│   └── package.cmake     # builds the dist/ zip for release
├── dist_readme.txt       # README.txt shipped inside the zip
├── init.py               # adds the build matching the running Nuke
├── menu.py               # 3D > Lights toolbar entry
├── LICENSE               # MIT license
└── README.md             # This file
```

## Requirements

- Nuke 17.0 or later
- Windows x64
- ScanlineRender2

## License

MIT — see [LICENSE](LICENSE) for details.
