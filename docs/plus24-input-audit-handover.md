# GoldenEye 007 Plus 2.4 input investigation

## Status

The Plus 2.4 menu bug is **not fixed or verified** in this branch. Only the generic Mouse Injector naming correction has been implemented. Do not present the name-only DLL as a 2.4 compatibility fix, and do not replace the public release with it as though the menu work were complete.

The Windows cross-build and generic-label checks passed in GitHub Actions run 37203544956. The compiled label checks verify the original `Mouse Injector - Input Settings` dialog title and removal of the temporary `FreeFly-F3` suffix from the plugin name/About text. The binary32 camera guard remains present. No interactive emulator test was performed.

## Pinned inputs

- Maintained injector baseline: `342d01ec5c1cb6ac15e098f8294943e554590cc3`.
- Last published, user-confirmed injector SHA-256 in the preceding work: `4ff94a049c7d4d3e5c4bee54e408c7646ddde1a52fff408e06378aa65c2d7eb3`.
- Naming build source: `8a184a30f8ae7877d3546ca67096fb1e6be11c7e`.
- Upstream Plus source independently fetched from main: `c42295ba400386aac4140957b6f7fa9e53ab911d`, message `Update GoldenEye 007 Plus through R26Y`.
- Official upstream v2.4 release: tag `#15`, release ID `402734452`, published 2026-10-03. Its `goldeneye_007_plus.xdelta` asset has reported SHA-256 `c475748c0d1913f56f3aa7b9ac2474cc3b46a3d1824b1874d41fda23d3eb8642`.
- User supplied `goldeneye_007_plus_2.4.z64` (33,554,432 bytes). The current conversation's execution runtime returned ClientError before any local code/file reads. **No actual uploaded-ROM hash, disassembly, resolver result, or runtime test was obtained.** The upload is present; this is not evidence of a damaged ROM. The Files text reader also cannot parse this binary.

The release publication date is later than the main-source commit. This alone does not prove a mismatch. Compare compiled code with the actual image before asserting that source and ROM agree or disagree.

## Findings from the maintained injector source

1. `GE_FindMenuMaxPage` recognizes a specific constructor-dispatch instruction sequence and otherwise falls back to 27. A changed dispatcher can therefore exclude newly added menus.
2. `GE_ResolveLevelModifierProfile` and the native-menu adapter depend on `mapmaker.page`, although the Level Modifiers UI is an independent feature.
3. The current Level Modifiers handler assumes three relative pages after Map Maker, fixed category/list counts and fixed rectangles. This is layout-specific support, not arbitrary-menu compatibility.
4. `device.c` gates `GAME_Inject()` on `GAME_Status()`. `game.c` clears its current driver when Status fails. There is no independent native-controller fallback on that path. This can withhold ordinary controller updates as well as mouse injection on an unrecognised layout. **The exact cause in the supplied 2.4 image remains unverified.**
5. Existing build preparation modifies source files; repeating normal make after preparation may attempt to reapply F3 patches to already-modified files. The clean build passed; an incremental-build audit is still needed.

## Next work

Work against the actual supplied 2.4 ROM once a functioning execution runtime is available. Compare its core address-profile/dispatch/editor/Level Modifiers recognition with the last working image. Inventory every new menu route and all descendants, distinguishing native cursor, digital list, value editing, confirmation, editor camera, gameplay watch, and transitions.

Separate native button availability from optional game-memory injection. Discover or validate each adapted UI independently instead of using Map Maker page offsets. Retain bounded memory writes and cached discovery; never treat every unknown page as a known digital menu. A future ROM-provided versioned input/UI descriptor is a design option, not an implemented feature or guarantee.

Test actual production handlers against the real ROM and saved RAM. Check every row center/boundary/gap, scrolled list, disabled item, value decrement/increment, click/Back/held-button transition, focus/capture change, menu re-entry and stop/play lifecycle. Source-string checks or a successful compile alone do not establish these behaviors. Benchmark idle and moving input in the emulator before claiming the slowdown is fixed.

## Preserve

- Control style 1.2 Solitaire.
- Existing E interaction / R reload behavior.
- Confirmed editor mouse-look precision fix and older supported Plus layouts.
- Existing emulator depth-buffer, texture-scroll, Native Test exit and automatic EEPROM/SRAM/Controller Pak handling.
- Original generic Mouse Injector naming and DLL filename.
- Public release files, descriptions and hashes remain untouched during this investigation. Publish only after the intended candidate is verified and approved.
