# Checks the INSTALLED SoftBox (~/.nuke/SoftBox/<ver>/) loads and renders:
# prints the default Shadow Mode, About version and the DLL paths in use.
# Run with each Nuke:  Nuke17.x.exe -t -i test/check_install.py
import nuke, ctypes
from ctypes import wintypes
ver = '%d.%d' % (nuke.NUKE_VERSION_MAJOR, nuke.NUKE_VERSION_MINOR)
nuke.scriptOpen(r'C:/dev/SoftBox/test/soft_shadow_test.nk')
a = nuke.toNode('B_SoftBoxAreaLight')
print('CHK', ver, 'default mode', a['shadow_mode'].value())
a['width'].setValue(2); a['height'].setValue(2); a['intensity'].setValue(50); a['shadow_samples'].setValue(16)
print('CHK', ver, 'about', [k.value() for k in a.allKnobs() if k.Class() == 'Text_Knob' and 'SoftBox v' in str(k.value())])
w = nuke.nodes.Write(file='C:/dev/SoftBox/test/out/installed_%s.png' % ver, file_type='png'); w.setInput(0, nuke.toNode('Render_B_SoftBox')); nuke.execute(w, 1, 1)
k = ctypes.WinDLL('kernel32'); k.GetModuleHandleW.restype = wintypes.HMODULE
k.GetModuleFileNameW.argtypes = [wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD]
for d in ('AreaLight.dll', 'slrRectLight.dll'):
    b = ctypes.create_unicode_buffer(512); h = k.GetModuleHandleW(d); k.GetModuleFileNameW(h, b, 512)
    print('CHK', ver, d, b.value if h else 'NOT LOADED')
