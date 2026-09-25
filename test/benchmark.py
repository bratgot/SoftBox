# Renders test/soft_shadow_test.nk with Grid vs Stochastic settings from the
# local build (build/plugin/Release) and prints render times. Output EXRs go to
# test/out/. Run:  Nuke17.1.exe -t -i test/benchmark.py
import nuke, time, ctypes
from ctypes import wintypes

nuke.pluginAddPath(r'C:/dev/SoftBox/build/plugin/Release', addToSysPath=False)
nuke.scriptOpen(r'C:/dev/SoftBox/test/soft_shadow_test.nk')

area = nuke.toNode('B_SoftBoxAreaLight')
srB  = nuke.toNode('Render_B_SoftBox')
area['width'].setValue(2.0); area['height'].setValue(2.0); area['intensity'].setValue(50)
out = 'C:/dev/SoftBox/test/out'
import os; os.makedirs(out, exist_ok=True)

def render(tag, mode, spp, cam):
    area['shadow_mode'].setValue(mode)
    area['shadow_samples'].setValue(spp)
    srB['camera_sample_mode'].setValue(cam)
    w = nuke.nodes.Write(file='%s/%s.exr' % (out, tag), file_type='exr')
    w.setInput(0, srB)
    t0 = time.time(); nuke.execute(w, 1, 1); dt = time.time() - t0
    nuke.delete(w)
    print('RUN %-26s %6.2fs' % (tag, dt))
    return dt

runs = [
    ('ref_stoch_16x64',  'Stochastic', 16, '64(8x8)'),
    ('grid16_cam1',      'Grid',       16, '1'),
    ('grid64_cam1',      'Grid',       64, '1'),
    ('grid16_cam16',     'Grid',       16, '16(4x4)'),
    ('stoch1_cam1',      'Stochastic', 1,  '1'),
    ('stoch1_cam16',     'Stochastic', 1,  '16(4x4)'),
    ('stoch4_cam16',     'Stochastic', 4,  '16(4x4)'),
    ('stoch16_cam1',     'Stochastic', 16, '1'),
    ('stoch64_cam1',     'Stochastic', 64, '1'),
]
times = {}
for tag, mode, spp, cam in runs:
    times[tag] = render(tag, mode, spp, cam)

k = ctypes.WinDLL('kernel32')
k.GetModuleHandleW.restype = wintypes.HMODULE
k.GetModuleFileNameW.argtypes = [wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD]
for dll in ('AreaLight.dll', 'slrRectLight.dll'):
    buf = ctypes.create_unicode_buffer(512)
    h = k.GetModuleHandleW(dll)
    k.GetModuleFileNameW(h, buf, 512)
    print('DLL', dll, buf.value if h else 'NOT LOADED')
