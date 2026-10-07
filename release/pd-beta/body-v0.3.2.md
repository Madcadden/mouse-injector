## Mouse Injector v0.3.2 — Perfect Dark beta and Plus 2.4 update

The download has been refreshed with the working Perfect Dark beta DLL and GoldenEye Plus 2.4 free-camera fix. Use it with **[1964 GEPD Auto-Mod Edition v0.2.2](https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.2)**, which already includes this exact injector.

### Changes

- Adds **Perfect Dark NTSC 6.4 and PAL 28.7 beta/debug support**: normal and cursor mouse aiming, EyeSpy pitch and independent interaction/reload.
- Restores **GoldenEye Plus 2.4 Map Maker free-camera mouse look**, with the confirmed free-fly fix.
- Keeps **E for interaction/opening doors and R for reload** with the usual gameplay bindings.
- Retains automatic GoldenEye control/FOV/reload discovery, supported Perfect Dark settings discovery, Map Maker menu controls, remappable D-pad/shoulder buttons and existing INI settings. Normal W+S input is retained.
- Keeps the generic **Mouse Injector** name in the emulator's plugin list.

The matching emulator provides the beta controller-polling patches, PAL task/EEPROM handling and recognized EC development-cart repair. EC repair affects only the loaded ROM copy; the file on disk is unchanged. PAL retains its native timing. The Plus texture-scrolling and Native Test Mode exit fixes also require that emulator.

### Install

Close 1964 and back up your current files. Install the matching emulator update, or copy this `Mouse_Injector.dll` into its `plugin` folder if your emulator is already current. Select **Mouse Injector** as the input plugin. Keep your INI files and cold-boot the ROM; old save states can restore old code.

This download is the normal 32-bit Windows plugin. It contains no ROMs, game assets, save files or user settings.

### Map Maker controls

Defaults with the WASD input profile, for Josh's **[GoldenEye 007 Plus](https://github.com/Joshua-1248/GoldenEye-007-Plus)**:

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

GoldenEye Plus itself provides expanded 1–4-player local co-op. Online co-op is not included here.

### Compatibility

The Perfect Dark beta build and Plus free-fly fix have been confirmed working in gameplay. Automated checks also cover ROM signatures, input routing, patch ownership and source/build correspondence. Automatic discovery applies to recognized code layouts; missing or ambiguous matches are skipped. PD debug menus use keyboard/controller navigation.

The attached `Mouse-Injector-PD-Beta-Source.zip` is the exact build-source snapshot. GitHub's automatically generated source archives still refer to the historical tag; use the attached source ZIP or the exact source link below.

### Files and source

`Mouse-Injector-Automatic-Mod-Compatibility-v0.3.2.zip` contains the normal 32-bit Windows `Mouse_Injector.dll`.

SHA-256:

- ZIP: `a191f1e81f6e3750b6c414313990c12600009dfd0f664aa754d9be7e2479873d`
- `Mouse_Injector.dll`: `968c4b9ae71f98733ef9a6523086df0890d2267c33795747eaa2bdb175ca0929`

[Emulator build source](https://github.com/Madcadden/1964GEPD/tree/bf6b09853378a121719179128f4cc8dcface9957) · [Injector build source](https://github.com/Madcadden/mouse-injector/tree/eaa00c1da6e3e2923d6599a16579db53af7db5a7)

### Credits

Thanks to **Graslu** for 1964 GEPD and its releases/guides; **Stolen and Carnivorous** for the original Mouse Injector/GEPD work and rewrite; **Joel Middendorf (schibo) and Rice** for 1964 0.8.5; **Catherine Reprobate (NeonNyan), HackBond and Graslu** for later Perfect Dark/decomp work; **Ryan Dwyer** and the Perfect Dark decompilation contributors; and **Ryan C. Gordon and the ManyMouse contributors**.

Thanks to **Josh (Joshua-1248)** and the **GoldenEye 007 Plus** contributors for the mod, Map Maker and expanded local co-op.

Auto-Mod Edition changes by **Jamie McCadden** ( ϓØŁØ ֆШΔǤǤΞƝŞ ). Thanks also to the mod, plugin and texture-pack creators.

[Graslu's setup guide](https://www.youtube.com/watch?v=8mL0I__VMec) · [4K 60 FPS video demo](https://www.youtube.com/watch?v=rxWkLdgdcPA&t=81s)
