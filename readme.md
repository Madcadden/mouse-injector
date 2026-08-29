# MouseInjector Plugin for 1964GEPD


This is a fork with support for Perfect Dark decomp.

## Random-Eye-zer support

This fork also supports Murk-17's
[Random-Eye-zer: The True Randomizer](https://www.moddb.com/mods/random-eye-the-true-randomizer)
when used with a RandomEye-compatible 1964 GEPD build.

The plugin detects RandomEye using ROM CRC `B72EDF71/C22234D1` and selects a
separate verified address profile for:

- Player pointers and controls
- Camera, pause, and exit state
- Menu page and mouse cursor coordinates
- Tank and multiplayer state
- Intro state
- RandomEye's additional menu pages, including Randomiser Options

Retail GoldenEye, Goldfinger 64, and Perfect Dark retain their existing
behavior. Retail-only ROM injection hacks are deliberately not written into
RandomEye because the mod relocates that executable code; direct mouse,
keyboard, and controller injection still works.

The saved **Prevent RandomEye Debug Shortcut (W+S)** option is enabled by
default. When forward and backward are pressed together, it gives forward
movement priority and prevents RandomEye's debug menu from opening. Disable
the option to restore RandomEye's original W+S shortcut.

No ROMs, ROM patches, game assets, or save files are included.

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
