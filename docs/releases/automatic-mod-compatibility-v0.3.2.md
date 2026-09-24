## Changes in v0.3.2

Pause-menu mouse navigation is enabled **only while editing in GoldenEye 007 Plus's Map Maker**. Regular gameplay watch menus use keyboard/controller navigation in every GoldenEye ROM, including Plus's Native Test Mode. Front-end, multiplayer and confirmation menu mouse controls are unchanged.

**Josh's [GoldenEye 007 Plus](https://github.com/Joshua-1248/GoldenEye-007-Plus) is supported**, including mouse controls for its Map Maker and menus. GoldenEye Plus also includes **1–4-player local co-op**, developed by Josh and the mod's contributors.

Use this plugin with **[1964 GEPD Auto-Mod Edition v0.2.2](https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.2)**, which already includes the matching injector.

The Native Test Mode exit fix is included in the updated **[1964 GEPD Auto-Mod Edition v0.2.2](https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.2)**. Install its updated `1964.exe` and cold-boot the ROM; the Mouse Injector v0.3.2 DLL is unchanged.

## Features retained from v0.3

- Automatically resolves GoldenEye controls, FOV and supported reload locations, including relocated game code. Invalid or ambiguous matches are skipped.
- Adds Map Maker free-camera mouse look and pointer selection in the Basic/Advanced chooser and editor menu.
- Adds mouse navigation to GoldenEye 007 Plus's Map Maker editing pause menus, plus multiplayer and confirmation menus that use directional selection.
- Adds configurable D-pad and L/R shoulder bindings. Existing settings migrate without resetting your controls or FOV.
- Makes R usable for Plus's native interact/reload action during gameplay; E retains its native action.
- Improves Perfect Dark FOV/settings discovery.
- Clears held clicks across adapted menu transitions and pending mouse navigation when capture is lost.

Normal W+S input and older INI compatibility remain available.

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

The Map Maker's editing pause menus support mouse navigation. Regular gameplay watch menus, including the watch in Plus's Native Test Mode, retain keyboard/controller navigation. Other directional menus retain their existing mouse controls. Orbit mode keeps its keyboard controls. Mouse look in First Person View and Native Test Mode is unchanged.

## Compatibility

GoldenEye 007 Plus is supported, along with compatible GoldenEye and Perfect Dark code layouts.

**Previously tested:** Retail GoldenEye, Murk's RandomEye-zer v1.1, Netplay 60FPS LTK Cup Edition v1.1, Cartridge Tilt, GE Stereo SFX, RickRollEye 64, Tomorrow Never Dies 64 Expanded, Goldfinger 64, GE Compilation 1.1, GoldenEye Tower, Pheonaarx's Yet To Come, Project GoldenEye v2.1 and TSWLM 64 Demo v1.

Automatic discovery supports recognized code layouts. Please open an issue if a mod does not work or has glitches. Include its name/version, required base ROM and what happened. Do not upload ROM files. Mod-specific bugs should also be reported to the mod's developer.

Online co-op with a full-screen view for each player is a possible future project; it is not included in this release.

## Installation

1. Close 1964 completely and back up your current DLL and INI.
2. Copy `Mouse_Injector.dll` from the standalone ZIP into the emulator's `plugin` folder, or install [1964 GEPD Auto-Mod Edition v0.2.2](https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.2), which already includes it.
3. Select Mouse Injector as the input plugin and cold-boot the ROM. Keep your existing settings file.

Back up the INI if you plan to return to an older plugin. No ROM files, ROM patches, game assets or save files are included.

## Credits

Thanks to **Stolen and Carnivorous** for the original Mouse Injector/GEPD work and rewrite; **Graslu** for 1964 GEPD and its releases/guides; **Catherine Reprobate (NeonNyan), HackBond and Graslu** for later PD/decomp work; **Ryan Dwyer** and the Perfect Dark decompilation contributors; and **Ryan C. Gordon and the ManyMouse contributors**.

Thanks to **Josh (Joshua-1248)** and the **GoldenEye 007 Plus** contributors for the mod, Map Maker and expanded co-op.

Auto-Mod Edition changes by **Jamie McCadden** ( ϓØŁØ ֆШΔǤǤΞƝŞ )

Thanks also to every mod, plugin and texture-pack creator!

## Files

The download has been updated in place; the version remains v0.3.2.

`Mouse-Injector-Automatic-Mod-Compatibility-v0.3.2.zip` contains the normal 32-bit Windows `Mouse_Injector.dll`.

**SHA-256**

- ZIP: `6767e25de0de2f910647a3f4aca3e7e3d4e526602dca920e69079faad0ecc963`
- `Mouse_Injector.dll`: `9d1c3df81c70601ff9bb7e7dd97c8cfa641ef3104f844d2c4d6e727468c798fd`

[Source](https://github.com/Madcadden/mouse-injector/tree/automatic-mod-compatibility) · [Updated build source](https://github.com/Madcadden/mouse-injector/tree/fecf88b0059531c42c73965514f9f7907a17d0a8)

