# 1964GEPD / Mouse Injector — GoldenEye 007 Plus 2.4 handover

Prepared 2026-10-04. Scope: emulator/injector input compatibility, not the separate GoldenEye–Perfect Dark AI ROM project.

## 1. Read this first

**GoldenEye 007 Plus 2.4 menu compatibility is NOT fixed. The actual uploaded 2.4 ROM has NOT been read, hashed, disassembled or run in the latest investigation. Only the generic Mouse Injector naming correction is implemented on the investigation branch.**

The user reported that additional menus were not behaving correctly, then that a newer Plus update no longer worked with the current injector. They supplied `goldeneye_007_plus_2.4.z64`. Do not substitute the earlier successful Level Modifiers tests for tests of this image.

The previous conversation's local execution environment failed with `ClientError` before even basic file reads. Both Python and container access failed again while preparing this handover. GitHub reads/writes and GitHub Actions remained available. This is not evidence of a damaged ROM, a wrong control setting, or a missing upload. Moving to a new conversation is a recovery attempt, not a guarantee that its execution environment will work.

This handover/export must not rebuild or change the emulator/injector, merge the investigation branch, update a release, or claim additional compatibility progress. Existing source and existing build artifacts are preserved as references.

## 2. User's current request and constraints

The user wants the latest supplied Plus 2.4 ROM to work correctly throughout its added menus and all descendants: pointer selection, selection highlights, acceptance, Back, option adjustment, scrolling, confirmation, editor camera and transitions. Their broader goal is an Auto-Mod emulator/injector that adapts to supported mods and menu additions without repeated per-version fixes.

They also explicitly want the original generic Mouse Injector identity. Remove temporary FreeFly/FreeFly-F3 labels from plugin-list, settings and About UI, while preserving the working camera fix and `Mouse_Injector.dll` filename. A generic title is not permission to remove editor functionality.

Keep control style **1.2 Solitaire**. The required gameplay mapping is **E for interaction/doors and R for reload**. Do not advise changing to 1.1 to hide the bug. Free Fly, not Orbit, was confirmed during the earlier editor-camera issue.

Existing defaults that must not be reset include Mouse/WASD free-camera look/movement, left-click placement, E delete in the editor, U for N64 L, O for N64 R, I/K/J/L for D-pad Up/Down/Left/Right, and Enter for the editor menu. Right-click has a contextual role: Back in adapted menus; preserve the established editor shoulder/speed behavior when editing. Retain custom primary/secondary bindings and `mouseinjector.ini` migration behavior rather than hardwiring defaults.

An earlier explicit scope restriction remains important: mouse navigation belongs in the Map Maker editing pause menus, not ordinary gameplay watch menus, including the Native Test gameplay watch. The latest request for all added menus needs to be addressed without silently undoing this distinction. First Person View is an editor preview with independent camera angles; Native Test is gameplay.

Preserve the existing `1964.exe` fixes: GoldenEye graphics/depth-buffer profile handling, texture-scroll/J-key protection, Native Test exit handling, and automatic save-device selection. Preserve older supported GoldenEye/Plus and Perfect Dark input behavior.

When a candidate is confirmed and publication is authorized, update the current Auto-Mod releases in place. Keep version numbers and descriptions unchanged except the existing hash values. No feature announcements or iterative changelog additions. Verify files, embedded and external checksum lists, and downloaded release bytes. Do not publish full ROMs, private save states or user configuration files.

The user has already endured long investigations and multiple candidates. Start from these references; avoid repeatedly asking whether they selected Free Fly or 1.2, avoid lengthy status polling without useful work, and distinguish implemented, statically checked, simulated, actually run, and user-confirmed behavior.

## 3. Repositories, pinned commits and recovery references

| Purpose | Repository / ref |
|---|---|
| Maintained emulator | `Madcadden/1964GEPD`, branch `automatic-mod-compatibility` |
| Emulator branch checked for this handover | `2c504607979068736f7e711d564811c095e3906f` |
| Maintained injector / prior published source baseline | `Madcadden/mouse-injector`, `342d01ec5c1cb6ac15e098f8294943e554590cc3` |
| Current investigation branch | `Madcadden/mouse-injector`, `fix/generic-name-plus24-audit` |
| Investigation snapshot before this expanded handover | `82cd88c44a975c9b196b35c143545d415504c90a` |
| Naming-only build source | `8a184a30f8ae7877d3546ca67096fb1e6be11c7e` |
| Earlier concise handover | `docs/plus24-input-audit-handover.md` on that investigation branch |
| Previous Level Modifiers development branch | `fix/plus-appended-menu-navigation` in the injector repo; PR #8 merged into the maintained branch |
| Official Plus repository | `Joshua-1248/GoldenEye-007-Plus` |
| Last independently retrieved Plus main-source snapshot | `c42295ba400386aac4140957b6f7fa9e53ab911d`, `Update GoldenEye 007 Plus through R26Y` |

The official Plus v2.4 release was recorded as tag `#15`, release ID `402734452`, published 2026-10-03. Its `goldeneye_007_plus.xdelta` asset had reported SHA-256 `c475748c0d1913f56f3aa7b9ac2474cc3b46a3d1824b1874d41fda23d3eb8642`. **This is an xdelta asset hash, NOT the uploaded ROM hash.** A release newer than the main-source commit does not establish source/ROM agreement or disagreement. Recheck upstream and compare relevant compiled routines against the uploaded image.

Current release references:

- Emulator: `Madcadden/1964GEPD`, tag `automatic-mod-compatibility-v0.2.2`, release ID `396012238`.
- Standalone injector: `Madcadden/mouse-injector`, tag `automatic-mod-compatibility-v0.3.2`, release ID `396010717`.

Prior confirmed publication hashes (September 30; verify again before any write):

| File | SHA-256 |
|---|---|
| `1964GEPD-Automatic-Mod-Compatibility-v0.2.2.zip` | `a2fd3e871db93a613ead2eb1f1fd71ed38cf4a5234ee2f9a1f58089c0dc31ab4` |
| `1964.exe` inside that bundle | `34ab4aa5dc5ded7a9e04f6950f464e43d6d26b44fa7758e3307406d663fffb32` |
| Confirmed `Mouse_Injector.dll` | `4ff94a049c7d4d3e5c4bee54e408c7646ddde1a52fff408e06378aa65c2d7eb3` |
| `Mouse-Injector-Automatic-Mod-Compatibility-v0.3.2.zip` | `43246fa3f1248c9ce2bce721684865ba9fb48228ef9b17b7420b847675e51179` |
| `1964GEPD-Automatic-Mod-Compatibility-v0.2.2-Source.zip` | `67a0441b14c5fe4e949cb2984a6232fad3a3fb4c04d3765cc3055b8d4d3c431f` |
| Release asset `Mouse-Injector-FreeFly-F3-Source.zip` after the September 30 replacement | `6cb542b520b7848e6ab6af0b8d73b70be743db3ebf6e035c5a6b60d3982ecf26` |

The historical source-archive name retained FreeFly-F3 even after it was updated in place. Do not identify its contents or the installed DLL by its name alone. Release descriptions deliberately retain older source hyperlinks; the explicit commits and downloaded bytes are the recovery anchors.

## 4. Files to bring into the new conversation

**Required:** this handover/recovery bundle and the actual `goldeneye_007_plus_2.4.z64` supplied by the user. Its recorded upload size is 33,554,432 bytes. Its original conversation path was `/mnt/data/goldeneye_007_plus_2.4.z64`; discover/confirm the real path after attaching it in the new conversation. No actual ROM hash is available from this investigation.

Useful older references already attached earlier include `goldeneye_007_plus_v3.z64`, `GoldenEye 007 Plus-usa.sav1`, `GoldenEye 007 Plus-usa(7).sav1`, `GoldenEye 007 Plus-usa(8).sav1`, `GEPD-FreeFly.log`, `1964GEPD-Plus-Level-Modifiers-Direct-Cursor-Fix-v2.zip`, and `Level-Modifiers-Direct-Cursor-Verification.json`. Repeated filenames are not reliable version identities. Hash and match any old ROM/state pair before using it. An old Level Modifiers state is not a Plus 2.4 state and should not be loaded into an unrelated ROM version.

The recovery export is intended to include verified repository snapshots, existing naming-only build evidence, and prior release packages. Read its machine-generated export manifest for what was actually recovered. It deliberately does not include ROMs, private saves, the actual installed INI, or the unrelated AI checkpoint archives. Source can also be recovered directly from the pinned repositories.

If actual device/capture behavior remains unexplained after the real ROM resolver and production input paths are tested, ask for the installed DLL, `mouseinjector.ini`, and a state or brief clip from the failing 2.4 menu. These are targeted follow-up diagnostics, not prerequisites to repeating questions already answered.

## 5. History that explains the current state

### A. Emulator graphics, texture and save work — preserve, do not restart

The user requested applying the retail GoldenEye depth-buffer/profile behavior across recognized GoldenEye mods instead of continually adding ROM-specific exceptions. They later confirmed the depth-buffer issue appeared fixed. Separate reports remain: Frigate co-op could freeze with the emulator unresponsive and Aztec geometry could disappear for the trailing player when the other moved ahead. Those were not conclusively assigned to ROM portals, emulator or renderer; do not certify them resolved by the input work.

Earlier J-key texture scrolling crashed in Map Maker. The emulator, rather than just the input binding, contains texture-selection bounds/reverse-scroll and IA4 decoding corrections. A previous cold-boot audit reported 26 instruction changes on an older supplied Plus image. That is historical evidence, not validation of 2.4. Preserve these changes and verify current signatures separately if needed. Old unpatched save states can bypass boot-time corrections.

Save hardware changed between Plus builds. Older builds used SRAM; the newer previously inspected builds required 16 Kbit EEPROM (2,048 bytes), with Map Maker map persistence using Controller Pak separately. Automatic selection was added to the emulator instead of globally forcing every ROM to 16 Kbit. It was intended to override stale explicit 4 Kbit settings for recognized newer Plus while retaining older SRAM support and Controller Pak availability. Do not conflate bits and bytes or assume the newest build's save routines without checking them.

### B. Free Fly input — previously solved, retain the actual fix

Arrow keys reached the native N64 stick while mouse camera injection did not work, even with 1.2 Solitaire and Free Fly selected. The diagnostic log showed mouse motion reaching Player 1 but camera validation rejecting every update. The cause was an exact floating-point comparison affected by 32-bit x87 excess precision. The correction validates stored binary32 pitch-limit words rather than relying on that comparison. The user confirmed the F3 candidate worked.

The camera guard checks include `0xBFC90FDAU` and `0x3FC90FDAU`. Preserve the correction and validation; do not disable safety guards or change global floating-point flags as a shortcut. FreeFly-F3 was temporary diagnostic branding, not the desired permanent plugin identity.

### C. Level Modifiers — earlier fix worked only for the old layout

A subsequent Plus ROM added `Special Options -> Level Modifiers -> category -> level list -> detail/toggle`. The initial solution assumed pages appended after Map Maker were digital menus. It did not deliver actual cursor hover behavior. A later pulse-based approximation made the highlight chase the cursor, selected wrong rows and was followed by a report of massive lag. A direct-selector candidate replaced that approximation. The user eventually confirmed the Direct-Cursor-v2 package worked and authorized publication on September 30.

Historical details for that old ROM only: Map Maker page 30, Level Modifiers pages 31–33; category/level/top globals `0x8006E800`, `0x8006E804`, `0x8006E808`; category rectangles X 70–370 with Y 84–104, 114–134 and 144–164; level rows used 16-pixel spacing with ten visible rows. The old saved cursor at approximately X 272.699/Y 109.725 lay in a category gap. Do not use those addresses or rectangles as universal facts or as verified 2.4 values.

The old source implemented Silo / Beta Vent Start and Miscellaneous / Citadel / Water, with many other level entries intentionally unavailable. Recheck this on 2.4; do not dismiss current broken controls based on the old disabled-item list.

Previous source audits and compile results did not prove full interactive behavior. In particular, the 12/12 and 16/16 menu audits included source-string/structural checks. Removal of synthetic pulses was proposed to address lag, but no measured emulator-wide root cause or performance result was established. Treat performance as a regression requirement.

## 6. Latest completed work: naming only

The investigation snapshot `82cd88c44a975c9b196b35c143545d415504c90a` is four commits ahead of maintained baseline `342d01ec5c1cb6ac15e098f8294943e554590cc3`. The compared changes are limited to:

- `tools/freefly-trace.patch`: removed the temporary UI/name overrides.
- `tools/fix_freefly_precision.py`: corresponding naming-related preparation adjustment.
- `tools/prepare_freefly_f3.py`: expected prepared-source hash adjustment.
- `.github/workflows/generic-name-plus24-audit.yml`: isolated build/label audit.
- `docs/plus24-input-audit-handover.md`: concise status and next steps.

The intended resulting settings title is `Mouse Injector - Input Settings`; plugin/About branding is generic Mouse Injector. Existing camera precision handling remains.

Existing GitHub Actions build: run `37203544956`, source `8a184a30f8ae7877d3546ca67096fb1e6be11c7e`. Artifact `11303597272`, `generic-name-and-menu-audit`, reported digest `sha256:eae25bdd5f1e379a7e6b43794baf79c189e1d2ecd816fd4bf4cf8555eab86510`, expires 2026-10-18. It contains naming-only source/build evidence, `name-verification.json`, `menu-audit-status.json`, and a ZIP explicitly marked NOT a Plus 2.4 fix. Download it before expiry or use the recovery bundle's preserved copy if export succeeded.

Passed: clean Win32 cross-build, compiled generic-label checks and presence of the camera binary32 guard. Not performed: interactive emulator input test or actual 2.4 image verification. Nothing from this naming investigation was published to the normal releases.

## 7. Source findings versus unverified hypotheses

These are findings from source inspection, not a proven diagnosis of the uploaded 2.4 image:

1. `GE_FindMenuMaxPage` recognizes a specific compiled constructor-dispatch sequence and otherwise falls back to 27. Menu-table changes can therefore reject later pages.
2. Level Modifiers discovery and native menu selection depend on `profile->mapmaker.page`, even though Level Modifiers is independent of Map Maker. A failed editor match can unnecessarily disable other menus.
3. The Level Modifiers adapter assumes three relative pages, fixed row counts and fixed geometry. Appended page order is not a general input contract. An arbitrary new menu may use cursor input, digital input, different controls or no user input at all.
4. `device.c` calls `GAME_Inject()` only when `GAME_Status()` passes; `game.c` clears its active driver on Status failure. There is no independent native-controller fallback on that path. Optional memory-injection failure can therefore withhold ordinary controller updates. Reproduce whether this is actually happening in 2.4.
5. Source preparation mutates tracked files. A clean build passed, but repeating normal make after preparation may attempt to reapply old F3 patches to modified files. The Level Modifiers preparation step and generic-name changes must remain coherent. Incremental-build behavior still needs verification.

Useful code map:

| File / subsystem | Investigation purpose |
|---|---|
| `device.c` | Polling, mouse capture/device filtering, status gate, injection lifecycle |
| `games/game.c` | Driver recognition, Status and driver clearing |
| `games/goldeneye.c` | Core/address resolver, menu limit, camera injection, controller assembly, adapter order |
| `games/goldeneye.menunav.h` | Native digital menu routing and pulse fallback |
| `games/goldeneye.mapmenu.h` | Editor/chooser hover and click routing |
| `games/goldeneye.mapmaker16.h` | Revised editor layout and First Person preview matching |
| `tools/apply_plus_levelmod_cursor.py` | Generated Level Modifiers resolver and direct selector integration |
| `tools/prepare_freefly_f3.py`, `tools/mapmaker-input-compat.patch`, `tools/freefly-trace.patch`, `tools/fix_freefly_precision.py` | Prepared versus raw source; naming and camera fix |
| `maindll.c`, `ui/ui.rc`, `makefile` | Generic identity, resources and build preparation |
| `tests/test_plus_appended_menus.py`, `tests/test_goldeneye_resolver.py` | Existing test coverage and old hardcoded fixture assumptions |
| Plus `src/game/front.c`, `mapmaker.c`, `options.c`, `mpmenu.c`, `levelmodifiers.c`, `src/bondconstants.h` | Actual menu implementations, descendants, transitions and control semantics |
| Emulator `ge_save_profile.h`, `1964ini.c`, `iPIF.c`, `dma.c`, `win32/Wingui.c` | Existing compatibility changes to preserve; not an instruction to edit all of them |

## 8. What must be done next, in order

### Step 1 — establish a working runtime and exact baselines

Before coding, test basic local execution and file access. Inspect the newly attached ROM, record its size, byte order and SHA-256, then preserve an untouched copy. Do not materialize automatically mounted attachments merely as a ritual. If execution still fails, state that promptly; do not claim disassembly or repeatedly spend the session retrying the same broken path.

Recover the pinned naming/audit source and maintained baseline. Review the handover and patch preparation. Keep the published EXE/DLL as rollback references. Verify hashes rather than trusting package labels or assumed installation state.

### Step 2 — reproduce actual 2.4 recognition failures

Run the production resolver against the actual uploaded image. Record game-family identification, menu global and maximum, dispatcher evidence, editor/chooser/camera recognition and each extra-menu adapter. Determine exactly which checks succeed or fail and why. Check every `Status` gate that can stop native buttons. Compare old/new code and current upstream sources; do not simply relax masks until something matches.

### Step 3 — inventory the menu graph before broad input changes

Trace every reachable entry and submenu from main/Special Options and Map Maker. Record the live page/state identifiers, row selector, counts, scroll offsets, enable predicates, renderer geometry, accepted buttons and return paths. Distinguish native cursor menus, digital lists, toggles, sliders/value editing, confirmations, editor camera, gameplay watches and noninteractive transitions. The new inventory must come from 2.4, not the old pages-31–33 fixture.

### Step 4 — separate safe controller fallback from optional memory injection

Ordinary configured N64 buttons/stick updates must not depend on an optional camera/menu-memory signature remaining recognized. Design the fallback without retaining stale pressed keys or writing to unknown RAM. Preserve per-player device mapping, disabled profiles, capture/focus rules and the actual selected plugin protocol. Do not blindly apply GoldenEye memory writes to unknown ROMs or assume all unknown pages use D-pad semantics.

Resolve independent UI adapters independently. Validate menu-table bounds instead of falling back in a way that rejects legitimate new pages. Cache expensive discovery by actual ROM identity/lifecycle rather than rescanning each input tick or menu entry. Keep all memory writes bounded and based on corroborated code/data evidence.

### Step 5 — implement pointer semantics against real state and rendering

For adapted lists, use the live selector/scroll state and the rendered coordinate system. Hover must highlight the item actually under the cursor, not an independently guessed row. Do not activate the previous row when clicking a gap, header or disabled item. Account for scroll offsets, list ends, re-entry, keyboard changes and any menu scaling. Preserve native actions for toggles/sliders/confirmation instead of making every click mean the same thing.

Remove or isolate conflicting gesture/cursor owners. Do not restore the earlier continuous pulse-chasing workaround as the main fix. Ensure one click is not carried into the next submenu or gameplay. Generic fallback should be useful but must not pretend it knows arbitrary future hitboxes.

A versioned ROM-provided input/UI descriptor could enable genuine long-term automatic adaptation. It is a design option only, not an implemented feature. Supporting an unknown future menu with arbitrary controls cannot be guaranteed solely by assuming its page follows Map Maker. Deliver supported 2.4 compatibility first while making unsupported cases safe and diagnosable.

### Step 6 — verify, package a candidate, then publish only when approved

Execute actual production code in tests using real ROM data and appropriate saved RAM. Include both normal and relevant alternate builds; verify clean and repeated/incremental builds. Do not inflate lexical source checks into runtime claims. Then perform available emulator testing and report separately what remains for the user's interactive test.

Keep candidate filenames generic and replace only the necessary component. Usually this is `plugin/Mouse_Injector.dll`; change `1964.exe` only if reproduced emulator-side behavior requires it. Leave INI, saves, ROM and graphics plugin unchanged unless an explicit migration is justified.

## 9. Acceptance checks

| Area | Required evidence |
|---|---|
| Detection | Actual 2.4 core and menu recognition; later pages do not kill fresh controller updates; invalid/ambiguous memory targets rejected |
| Hover | Every active row center selects that row; edges/gaps/headers/outside-list space do not select or activate a different entry |
| Lists | All visible rows and every scroll offset; top/bottom limits; shorter categories; disabled entries; keyboard/mouse mixed use |
| Actions | Correct Accept/Back/decrement/increment/toggle; confirmation Cancel; no off-row or held-entry accidental activation |
| Transitions | Click release, menu re-entry, saved state restoration, capture loss/reacquisition, Stop/Play and ROM switching |
| Editor | Free Fly, separate First Person preview, chooser and editing menu; D-pad texture/module/layer changes and L/R shoulders |
| Gameplay | 1.2 Solitaire; E/R requirements; aiming/fire unchanged; ordinary and Native Test watches retain their intended keyboard/controller scope |
| Performance | Measured idle/menu-entry/mouse-movement cost; no repeated ROM scans, excessive tracing, synthetic pulse loops or accumulating state |
| Existing fixes | No regression of depth-buffer, texture/J-key, Native Test exit or save-device/Controller Pak behavior; no unverified claim that all still match 2.4 |
| Identity | Generic plugin list/About/settings title; original DLL filename; no temporary FreeFly labels |
| Distribution | Exact tested bytes; source matches distributed code; hashes match downloaded ZIPs and contents; release descriptions unchanged apart from hashes |

## 10. Publishing and build pitfalls to avoid

The last successful publication updated emulator v0.2.2 and standalone injector v0.3.2 in place. The two repositories require their own authorized write path: a GitHub Actions token from the emulator repo failed with HTTP 403 when attempting to overwrite the injector repo's release assets. A workflow in the injector repo completed its own publication. Do not assume a token grants cross-repository release access.

Use the exact user-approved DLL, not a different rebuild silently substituted under the same filename. If rebuilding is necessary, identify that candidate and verify it. Fetch fresh release metadata before mutation, back up existing assets/body, update only intended files/hashes, and verify by re-downloading. Preserve CRLF/description text except hash replacements; avoid brittle regex that misses carriage returns. Do not move old source tags to make retained hyperlinks look newer.

The build uses 32-bit Windows MinGW and generated/prepared sources. Inspect `makefile` before running prep manually plus a default make that reruns it. The raw Git snapshot alone may omit generated source changes until preparation. A source ZIP must clearly say whether it contains raw source plus preparation or actual prepared compilation inputs.

Old upstream compiler attempts failed because a bundled IDO tool was not a valid expected MIPS binary, an optional `src/fastbuild_abi.h` was absent, and trying to compile unrelated full frontend source with GCC exposed ABI/header errors. Do not repeat that detour merely to guess variable addresses when the actual ROM can be inspected. Derive evidence from matching production code and real input data.

## 11. Evidence boundaries and source links

The user-confirmed history above is drawn from the preceding project conversation. Architectural findings and naming status are recorded in the pinned concise handover and build workflow. They do not certify behavior of the new ROM.

- Investigation snapshot: https://github.com/Madcadden/mouse-injector/tree/82cd88c44a975c9b196b35c143545d415504c90a
- Concise handover: https://github.com/Madcadden/mouse-injector/blob/82cd88c44a975c9b196b35c143545d415504c90a/docs/plus24-input-audit-handover.md
- Naming build and checks: https://github.com/Madcadden/mouse-injector/actions/runs/37203544956
- Maintained injector baseline: https://github.com/Madcadden/mouse-injector/tree/342d01ec5c1cb6ac15e098f8294943e554590cc3
- Emulator source snapshot: https://github.com/Madcadden/1964GEPD/tree/2c504607979068736f7e711d564811c095e3906f
- Auto-Mod emulator release: https://github.com/Madcadden/1964GEPD/releases/tag/automatic-mod-compatibility-v0.2.2
- Standalone injector release: https://github.com/Madcadden/mouse-injector/releases/tag/automatic-mod-compatibility-v0.3.2
- Upstream Plus: https://github.com/Joshua-1248/GoldenEye-007-Plus

**Resume objective:** investigate and fix actual Plus 2.4 menu/input compatibility using this preserved baseline, keep the generic naming correction, avoid earlier layout and publication mistakes, and provide a verifiable candidate without sacrificing existing functionality.
