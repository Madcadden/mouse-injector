# MouseInjector Plugin for 1964GEPD


**[Download Mouse Injector v0.3.2](https://github.com/Madcadden/mouse-injector/releases/tag/automatic-mod-compatibility-v0.3.2)**

This fork provides automatic mod compatibility for GoldenEye and Perfect Dark, alongside the separate Perfect Dark decomp build. Use the normal plugin with **[1964 GEPD Auto-Mod Edition v0.2.2](https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.2)**, whose ZIP already includes the matching injector.

## Map Maker pause-menu mouse navigation

Pause-menu mouse navigation is enabled **only while editing in GoldenEye 007 Plus's Map Maker**. Regular gameplay watch menus use keyboard/controller navigation in every GoldenEye ROM, including Plus's Native Test Mode. Front-end, multiplayer and confirmation menu mouse controls are unchanged.

The Native Test Mode exit fix is included in the updated **[1964 GEPD Auto-Mod Edition v0.2.2](https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.2)**. Install its updated `1964.exe` and cold-boot the ROM; the Mouse Injector v0.3.2 DLL is unchanged.

## GoldenEye ROM-mod support

This fork resolves GoldenEye addresses and injection points from unique ROM
code patterns at runtime. This supports compatible ROM mods without requiring
a CRC-specific profile for each release, including Murk-17's
[Random-Eye-zer: The True Randomizer](https://www.moddb.com/mods/random-eye-the-true-randomizer).

The automatically resolved data includes:

- Player pointers and controls
- Camera, pause, and exit state
- Menu page and mouse cursor coordinates
- Tank and multiplayer state
- Intro state
- Reload injection points

Legacy retail-only ROM patches are written only when their original
instructions are present. Direct mouse, keyboard, controller, and reload
support use the automatically resolved layout.

No ROMs, ROM patches, game assets, or save files are included.

## FOV and Perfect Dark discovery

GoldenEye FOV, related viewmodel data and supported controller/aim patches are
resolved independently. GoldenEye 007 Plus uses different pause and multiplayer
globals; these are resolved from code references. At cold boot the matching
emulator prepares ROM code before the mod copies it into RAM.

Perfect Dark runtime globals, FOV/zoom and supported settings are found from
unique instruction windows. The legacy PD cursor/reload trampolines require
their fully verified canonical layout. Rewritten Plus reload, reverse-pitch,
aspect/HUD patterns are skipped rather than receiving retail replacements.
For the recognized Plus layout, R instead uses its validated native B-button
interact/reload path during active gameplay. It takes priority over Fire while
held to avoid the mod's B+Z holster combination. E keeps its native behaviour;
the separate reload-only trampoline has not been ported.

Missing or ambiguous code patterns are skipped. Automatic discovery does not
guarantee compatibility with every rewritten mod. Upgrade both the emulator
and injector, then cold-boot the ROM; old save states may retain old code.

### GoldenEye Plus Map Maker

This release supports Josh's **[GoldenEye 007 Plus](https://github.com/Joshua-1248/GoldenEye-007-Plus)**, including its Map Maker. The mod itself includes expanded **1–4-player local co-op**. Thanks to Josh and the GoldenEye Plus contributors. Online co-op with a full-screen view for each player is a possible future project, not a feature of this release.

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

The active tool determines the D-pad action, including module, layer or texture selection. Keyboard **R remains reload** during gameplay; it is separate from the N64 R shoulder. Arrow keys still supply analog-stick input.

Free-camera mouse look uses your sensitivity, acceleration and invert-pitch settings. It pauses while the editor menu is active. Orbit mode retains its native keyboard controls. Mouse look in First Person View and Native Test Mode is unchanged.

The Basic/Advanced chooser and editor menu support pointer selection and clicks. Click the left or right side of Material, Music and Grid Size values to adjust them. Mouse navigation while paused is limited to the Map Maker's editing menus. Regular gameplay watch menus in every GoldenEye ROM, including Plus's Native Test Mode, retain keyboard/controller navigation. Multiplayer and confirmation menu mouse controls are unchanged. Held clicks are released across menu transitions to avoid accidental placement or firing.

### Input settings

The normal plugin exposes all four D-pad directions and both shoulders, each with primary and secondary bindings. I/K/J/L and U/O are the defaults in both WASD and ESDF profiles. These buttons also reach the normal Perfect Dark driver.

Existing settings migrate without resetting old controls or FOV. New defaults are added to old custom profiles only when their keys are unused; conflicting additions stay unbound so you can assign them. Previously cleared bindings stay cleared. Back up your INI if you intend to return to an older plugin. The separate PD decomp configuration and defaults are unchanged.

See the [v0.3.2 release notes](docs/releases/automatic-mod-compatibility-v0.3.2.md) for installation, changes and file hashes.

## Building

`make clean` is required when builing a new Mouse Injector configuration

### Vanilla GE/PD

`make`

For a 32-bit Windows plugin from Ubuntu/WSL:

```bash
make -f makefile clean
mkdir -p obj
make -f makefile mouseinjector \
  CC=i686-w64-mingw32-gcc \
  WINDRES=i686-w64-mingw32-windres
```

The output is `Mouse_Injector.dll`. Because 1964 GEPD is 32-bit, the DLL must
also be built for 32-bit Windows.

The published normal DLL can also be built from the repository root with Zig 0.13.0:

```bash
python3 tools/build_injector_zig.py \
  --source . --zig /path/to/zig --output /path/to/build-output
```

The script records input/output hashes in its build manifest. Use `--speedrun` or `--pd-decomp` with separate output directories for those configurations.

### Vanilla GE/PD Speedrun build

`make SPEEDRUN_BUILD=1`


### Perfect Dark decomp build

`make PD_DECOMP=1`

## Note for decomp modders:

Include the Perfect Dark Mouse Injector decomp branch into your Perfect Dark decomp project and build with the appropriate build flags.

## Attribution
- Perfect Dark decompilation project by Ryan Dwyer
- Vanilla GE/PD Mouse Injector/1964GEPD by Stolen, rewritten by Carnivorous
- Mouse Injector for Perfect Dark decomp UI, proof of concept, building from linux; by Catherine Reprobate
- Injectable Mouse Injector "unofficial patches" for Perfect Dark originally written by Stolen/Carnivorous
    - These unofficial patches were ported to the Perfect Dark decompilation compatability patches by Catherine Repbrobate, Graslu, and HackBond
