#!/usr/bin/env python3
"""Audit appended GoldenEye Plus frontend menus against the injector policy."""
from pathlib import Path
import re, sys

root = Path(__file__).resolve().parents[1]
plus = Path(sys.argv[1]) if len(sys.argv) > 1 else None
if plus is None or not plus.exists():
    raise SystemExit("usage: test_plus_appended_menus.py <GoldenEye-007-Plus source>")

nav = (root / "games/goldeneye.menunav.h").read_text(errors="replace")
front = (plus / "src/game/front.c").read_text(errors="replace")
bc = (plus / "src/bondconstants.h").read_text(errors="replace")

checks = []
def ck(name, ok):
    checks.append((name, bool(ok)))
    print(("[PASS] " if ok else "[FAIL] ") + name)

menu = re.search(r"typedef enum MENU\s*\{(.*?)\}\s*MENU\s*;", bc, re.S)
body = menu.group(1) if menu else ""
order = [x for x in re.findall(r"\b(MENU_[A-Z0-9_]+)\b", body)
         if x not in ("MENU_INVALID", "MENU_MAX")]
for name in ("MENU_MAP_MAKER_BASIC","MENU_LEVEL_MODIFIERS",
             "MENU_LEVEL_MODIFIERS_LEVELS","MENU_LEVEL_MODIFIERS_DETAIL"):
    ck("menu enum contains " + name, name in order)
if all(x in order for x in ("MENU_MAP_MAKER_BASIC","MENU_LEVEL_MODIFIERS",
                            "MENU_LEVEL_MODIFIERS_LEVELS","MENU_LEVEL_MODIFIERS_DETAIL")):
    i = order.index("MENU_MAP_MAKER_BASIC")
    ck("Level Modifiers pages are appended after Map Maker",
       order[i+1:i+4] == ["MENU_LEVEL_MODIFIERS","MENU_LEVEL_MODIFIERS_LEVELS",
                         "MENU_LEVEL_MODIFIERS_DETAIL"])

ck("injector uses resolved appended-page range, not fixed Level Modifiers IDs",
   "page > (int)profile->mapmaker.page && page <= (int)profile->maxpage" in nav
   and "return 200 + page;" in nav
   and "page == 31" not in nav and "page == 32" not in nav and "page == 33" not in nav)

def fn(name):
    m = re.search(r"void\s+" + re.escape(name) + r"\s*\([^)]*\)\s*\{(.*?)\n\}", front, re.S)
    return m.group(1) if m else ""

rootfn = fn("interface_menu_level_modifiers")
levelsfn = fn("interface_menu_level_modifiers_levels")
detailfn = fn("interface_menu_level_modifiers_detail")

ck("category page accepts up/down", "U_JPAD | U_CBUTTONS" in rootfn and "D_JPAD | D_CBUTTONS" in rootfn)
ck("category page accepts A/Z/Start and B",
   "A_BUTTON | Z_TRIG | START_BUTTON" in rootfn and "B_BUTTON" in rootfn)
ck("level list accepts up/down", "U_JPAD | U_CBUTTONS" in levelsfn and "D_JPAD | D_CBUTTONS" in levelsfn)
ck("level list accepts A/Z/Start and B",
   "A_BUTTON | Z_TRIG | START_BUTTON" in levelsfn and "B_BUTTON" in levelsfn)
ck("detail page accepts left/right or A/Z/Start and B",
   "L_JPAD | R_JPAD | L_CBUTTONS | R_CBUTTONS" in detailfn
   and "A_BUTTON | Z_TRIG | START_BUTTON" in detailfn and "B_BUTTON" in detailfn)

# The parent Special Options page remains cursor-driven. This guarantees the
# new native adapter starts only after entering an appended page.
ck("Special Options entry remains cursor-clickable",
   "g_ModOptionsLevelModifiersHover" in front
   and "frontChangeMenu(MENU_LEVEL_MODIFIERS, FALSE)" in front)

bad=[n for n,ok in checks if not ok]
print()
print("PLUS APPENDED MENU AUDIT: %s (%d/%d)" %
      ("PASS" if not bad else "FAIL", len(checks)-len(bad), len(checks)))
if bad:
    for n in bad: print(" - " + n)
    raise SystemExit(1)
