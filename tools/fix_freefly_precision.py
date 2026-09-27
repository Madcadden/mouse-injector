#!/usr/bin/env python3
"""Apply the narrow F3 fix after the verified Map Maker/F2 integration."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
path = root / 'games/goldeneye.c'
text = path.read_text()
old = '''\t/* The native camera uses radians and clamps pitch to +/-pi/2. Verify
\t * the live constants too, so uninitialized or changed layouts fail shut. */
\tif(!isfinite(yaw) || !isfinite(pitch) || !isfinite(minimum) || !isfinite(maximum)
\t\t|| minimum != -1.570796251296997f || maximum != 1.570796251296997f
\t\t|| pitch < minimum || pitch > maximum)'''
new = '''\t/* These are N64 binary32 constants, not approximate decimal values.
\t * Strict C11/x87 may evaluate even f-suffixed decimal constants with
\t * excess precision, rejecting valid stored floats. Compare their exact
\t * words instead; retain finite-value, pitch-range and context guards.
\t * Do not change the process floating-point mode or loosen validation. */
\tif(!isfinite(yaw) || !isfinite(pitch) || !isfinite(minimum) || !isfinite(maximum)
\t\t|| (unsigned int)EMU_ReadInt(minaddress) != 0xBFC90FDAU
\t\t|| (unsigned int)EMU_ReadInt(maxaddress) != 0x3FC90FDAU
\t\t|| pitch < minimum || pitch > maximum)'''
if text.count(old) != 1:
    raise SystemExit('F2 camera validation does not match; nothing written')
path.write_text(text.replace(old, new), newline='\n')
for name in ('maindll.c', 'freefly_trace.h', 'games/goldeneye.freeflytrace.h'):
    path = root / name
    text = path.read_text()
    if 'FreeFly-F2' not in text:
        raise SystemExit('F2 build marker missing: ' + name)
    path.write_text(text.replace('FreeFly-F2', 'FreeFly-F3'), newline='\n')
print('Applied F3 binary32 validation; no emulator, save-device or graphics changes')
