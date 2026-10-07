# Mouse Injector for 1964 GEPD

**[Download Mouse Injector v0.3.2](https://github.com/Madcadden/mouse-injector/releases/tag/automatic-mod-compatibility-v0.3.2)** · **[Current source](https://github.com/Madcadden/mouse-injector/tree/automatic-mod-compatibility)**

Use the normal 32-bit Windows plugin with **[1964 GEPD Auto-Mod Edition v0.2.2](https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.2)**. The emulator ZIP already includes the matching DLL. The plugin appears as **Mouse Injector** in Input Settings.

The v0.3.2 download has been refreshed with the working Perfect Dark beta build and the GoldenEye Plus 2.4 free-camera fix.

## Latest input support

- **Perfect Dark NTSC 6.4 and PAL 28.7 debug/beta layouts:** normal/cursor mouse aiming, EyeSpy pitch and independent interaction/reload.
- **E = interact/open doors; R = reload** with the usual gameplay bindings, including supported Plus and beta layouts.
- **GoldenEye Plus 2.4 Map Maker free-camera mouse look**, retaining the confirmed free-fly fix.
- Automatic discovery of supported GoldenEye player/control, FOV, menu and reload locations, including compatible relocated code and [Murk's RandomEye-zer](https://www.moddb.com/mods/random-eye-the-true-randomizer).
- Perfect Dark runtime, FOV/zoom and settings discovery. Unsupported cosmetic or ambiguous patch patterns are skipped.
- Remappable D-pad and shoulder bindings, existing settings support and normal W+S input.

Install the matching emulator as well: its changes provide EC loaded-copy repair, PAL task handling/EEPROM settings and beta controller-polling patches. It also supplies the Plus texture-scrolling and Native Test Mode exit fixes. The EC repair leaves the ROM file on disk unchanged; PAL keeps its native timing.

## GoldenEye Plus Map Maker

Josh's **[GoldenEye 007 Plus](https://github.com/Joshua-1248/GoldenEye-007-Plus)** includes the Map Maker and expanded **1–4-player local co-op**. Online co-op is not included here.

Defaults with the WASD input profile:

| Action | Mouse / keyboard |
|---|---|
| Look / move in free camera | Mouse / WASD |
| Place or draw | Left-click / hold left-click |
| Delete | E |
| Rotate (N64 L shoulder) | U |
| 2× movement speed (N64 R shoulder) | Hold O or right-click |
| D-pad Up / Down / Left / Right | I / K / J / L |
| Open editor menu | Enter |
| Editor menus | Hover and left-click; right-click goes back |

The active tool determines the D-pad action, including module, layer or texture selection. Click either side of Material, Music and Grid Size values to adjust them. Bindings can be changed in Input Settings. Keyboard **R is reload during gameplay**, separate from the N64 R shoulder.

Mouse navigation while paused applies to Map Maker editing menus. Regular GoldenEye gameplay watch menus, including Plus's Native Test Mode, use keyboard/controller navigation. Front-end, multiplayer and confirmation menus retain their existing mouse controls. Orbit mode keeps its keyboard controls. Free-camera mouse look pauses while the editor menu is open.

Free-camera mouse look uses your sensitivity, acceleration and invert-pitch settings. Held clicks are cleared across adapted menu transitions to avoid accidental placement or firing.

## Settings and installation

1. Close 1964 and back up your current DLL and INI files.
2. Install the matching [emulator update](https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.2), or copy `Mouse_Injector.dll` from this standalone ZIP into its `plugin` folder if the emulator is already current.
3. Select **Mouse Injector** as the input plugin and cold-boot the ROM. Keep your existing settings.

The normal plugin exposes the four D-pad directions and both shoulders with primary/secondary bindings. I/K/J/L and U/O are their defaults in the WASD and ESDF profiles. Existing controls and FOV are preserved; new defaults are added only where their keys are unused. Previously cleared bindings stay cleared. Back up the INI before returning to an older plugin.

Old save states may restore unpatched game code. No ROMs, game assets or save files are included.

## Compatibility

The Perfect Dark beta build and Plus free-fly fix have been confirmed working in gameplay. Automated checks also cover ROM signatures, input routing, patch ownership and source/build correspondence. Automatic discovery applies to recognized code layouts; missing or ambiguous matches are skipped. PD debug menus use keyboard/controller navigation.

The separate speedrun and Perfect Dark decomp configurations remain source build options; the release download is the normal Windows plugin. See the [v0.3.2 release notes](docs/releases/automatic-mod-compatibility-v0.3.2.md) for hashes.

## Building

Build the current prepared source from the **`automatic-mod-compatibility` branch**. The published DLL uses Zig 0.13.0:

```bash
python3 tools/build_injector_zig.py \
  --source . --zig /path/to/zig --output /path/to/build-output
```

The helper records compilation input and output hashes. Use separate output directories with `--speedrun` or `--pd-decomp` for those configurations. Do not rerun the historical source-preparation scripts over the already prepared source.

The makefile also supports MinGW. Run `make clean` before changing configurations:

```bash
make -f makefile clean
mkdir -p obj
make -f makefile mouseinjector \
  CC=i686-w64-mingw32-gcc \
  WINDRES=i686-w64-mingw32-windres
```

The output must be a **32-bit Windows** `Mouse_Injector.dll` because 1964 GEPD is 32-bit. `make SPEEDRUN_BUILD=1` selects the speedrun configuration; `make PD_DECOMP=1` selects the decomp configuration. Decomp modders also need the matching Mouse Injector decomp integration in their Perfect Dark project.

[Emulator build source](https://github.com/Madcadden/1964GEPD/tree/bf6b09853378a121719179128f4cc8dcface9957) · [Injector build source](https://github.com/Madcadden/mouse-injector/tree/eaa00c1da6e3e2923d6599a16579db53af7db5a7)

## Credits

Thanks to **Graslu** for 1964 GEPD and its releases/guides; **Stolen and Carnivorous** for the original Mouse Injector/GEPD work and rewrite; **Joel Middendorf (schibo) and Rice** for 1964 0.8.5; **Catherine Reprobate (NeonNyan), HackBond and Graslu** for later Perfect Dark/decomp work; **Ryan Dwyer** and the Perfect Dark decompilation contributors; and **Ryan C. Gordon and the ManyMouse contributors**.

Thanks to **Josh (Joshua-1248)** and the **GoldenEye 007 Plus** contributors for the mod, Map Maker and expanded local co-op.

Auto-Mod Edition changes by **Jamie McCadden** ( ϓØŁØ ֆШΔǤǤΞƝŞ ). Thanks also to the mod, plugin and texture-pack creators.

Perfect Dark's original unofficial Mouse Injector patches were written by Stolen/Carnivorous and adapted for the decomp compatibility work by Catherine Reprobate, Graslu and HackBond.

[Graslu's setup guide](https://www.youtube.com/watch?v=8mL0I__VMec) · [Upstream 1964 GEPD](https://github.com/Graslu/1964GEPD)
