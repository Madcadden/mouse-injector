## About

Mouse Injector v0.3.1 fixes missing controls in GoldenEye PD-AI v2.42. Use it with the matching emulator below; startup and controls have been confirmed by user testing with both fixes installed.

It retains v0.3 support for Josh's **[GoldenEye 007 Plus](https://github.com/Joshua-1248/GoldenEye-007-Plus)**, with Map Maker mouse look and menu controls. GoldenEye Plus itself includes **1–4-player local co-op**, developed by Josh and the mod's contributors.

Use this plugin with **[1964 GEPD Auto-Mod Edition v0.2.1](https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.1)**. The emulator release already bundles this DLL.

## Changes since v0.3

- Fixes GoldenEye input detection when a rebuilt ROM places its two multiplayer round-state flags in the opposite memory order, as PD-AI v2.42 does.
- Retains instruction-pattern, aligned address-range, relative-layout and uniqueness checks. No fixed PD-AI checksum or flag address is required.
- Adds production-driver tests for boot/reopen, menu and gameplay controls, button releases, one-human/eight-Sim input isolation and round-end handling.

## Included v0.3 features

- Resolves GoldenEye FOV, related viewmodel data and supported control/aim settings independently, including GoldenEye Plus's relocated state.
- Adds free-camera mouse look in Map Maker, using your sensitivity, acceleration and invert-pitch settings.
- Adds pointer selection in the Basic/Advanced chooser and editor menu, plus mouse gestures for GoldenEye watch, multiplayer and confirmation menus that use directional selection.
- Adds configurable D-pad Up/Down/Left/Right and L/R shoulder inputs, with primary and secondary bindings. Existing settings migrate without resetting old controls or FOV; conflicting new defaults in custom profiles stay unbound.
- Makes R use Plus's native interact/reload action during gameplay. R takes priority over Fire while held to avoid the mod's B+Z holster combination. E retains its native action.
- Improves Perfect Dark runtime, FOV/zoom and supported settings discovery.
- Clears held clicks across adapted menu transitions and pending mouse navigation when capture is lost.

The v0.2 removal of the obsolete RandomEye W+S workaround is retained. Normal W+S input and older INI compatibility remain available.

## Map Maker controls

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

The active tool determines the D-pad action, including module, layer or texture selection. Click either side of Material, Music and Grid Size values to adjust them. Buttons can be remapped in Input Settings. **Keyboard R remains reload** during gameplay, separate from the N64 R shoulder.

Watch and other directional menus respond to mouse movements and left-click. Enter still closes the main watch. Orbit mode keeps its keyboard controls; test-preview mouse controls are not added.

## Compatibility

PD-AI v2.42 startup and controls have been confirmed by user testing with this injector and the matching emulator startup fix. This confirmation covers loading and input; it is not a claim that every PD-AI gameplay feature has been tested.

Automated tests exercise production code with supplied ROM data and bounded simulated input/memory. The PD-AI normal and speedrun tests pass boot/reopen, menu and gameplay input, presses/releases, patch restoration, multiplayer input isolation and round-end handling. Fourteen independent resolver cases also pass. Existing retail GoldenEye/Plus, Perfect Dark and INI/D-pad regression suites pass.

The included normal 32-bit Windows DLL is the same binary supplied for the successful user test. Its plugin exports, imports and settings resources were verified. GoldenEye Plus operation was also confirmed by user testing for v0.3; that release's features are retained.

**Previously tested:** Retail GoldenEye, Murk's RandomEye-zer v1.1, Netplay 60FPS LTK Cup Edition v1.1, Cartridge Tilt, GE Stereo SFX, RickRollEye 64, Tomorrow Never Dies 64 Expanded, Goldfinger 64, GE Compilation 1.1, GoldenEye Tower, Pheonaarx's Yet To Come, Project GoldenEye v2.1 and TSWLM 64 Demo v1.

Missing or ambiguous code patterns are skipped. Automatic discovery cannot guarantee every rewritten mod. Plus's reload key uses its native interact/reload action; the separate reload-only patch has not been ported. Unverified reverse-pitch and HUD/aspect code is left alone. Perfect Dark's legacy cursor/reload patches still require the verified canonical layout. These checks do not confirm gameplay compatibility with every Perfect Dark mod.

The mod's co-op feature is not a claim that every co-op configuration has been tested here. Online co-op with a full-screen view for each player remains a possible future project and is not included in this release.

## Installation

1. Close 1964 completely and back up your current DLL and INI.
2. Upgrade to [1964 GEPD Auto-Mod Edition v0.2.1](https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.1), which already includes this injector. If installing the standalone plugin ZIP, copy `Mouse_Injector.dll` into the emulator's `plugin` folder.
3. Select Mouse Injector as the input plugin and cold-boot the ROM without restoring an older save state. Keep your existing settings file; new bindings are added where their keys are unused. Use 8 MB / Expansion Pak for PD-AI.

Back up the INI if you plan to return to an older plugin. No ROM files, ROM patches, game assets or save files are included.

## Credits

Thanks to **Stolen and Carnivorous** for the original Mouse Injector/GEPD work and rewrite; **Graslu** for 1964 GEPD and its releases/guides; **Catherine Reprobate (NeonNyan), HackBond and Graslu** for later PD/decomp work; **Ryan Dwyer** and the Perfect Dark decompilation contributors; and **Ryan C. Gordon and the ManyMouse contributors**.

Thanks to **Josh (Joshua-1248)** and the **GoldenEye 007 Plus** contributors for the mod, Map Maker and expanded co-op.

Auto-Mod Edition changes by **Jamie McCadden** ( ϓØŁØ ֆШΔǤǤΞƝŞ )

Thanks also to every mod, plugin and texture-pack creator!

## Files

`Mouse-Injector-Automatic-Mod-Compatibility-v0.3.1.zip` contains the normal 32-bit Windows `Mouse_Injector.dll`.

**SHA-256**

- ZIP: `0fd8dc403093ed5be4fdcb514d44eafb0b2d6863f16394b67dc678768a77fdd7`
- `Mouse_Injector.dll`: `FB589DEE6590EACB002BF1EDEEE43B964968A273219D8E161B049CF9AE8DCC9F`

[Source](https://github.com/Madcadden/mouse-injector/tree/automatic-mod-compatibility) · [Release source](https://github.com/Madcadden/mouse-injector/tree/automatic-mod-compatibility-v0.3.1) · [PD-AI fix commit](https://github.com/Madcadden/mouse-injector/commit/dfb2de962088738948fc05272f300f2b7120f4c8)

