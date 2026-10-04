/* Temporary input-path diagnostics for the FreeFly-F3 candidate.
 * GPL-2.0-or-later. Records only game input routing and camera diagnostics.
 * Does not capture key names, key text, other windows or RAM/ROM contents.
 * Included in device.c; all state is owned by the input thread.
 */
#ifndef GEPD_FREEFLY_TRACE_H
#define GEPD_FREEFLY_TRACE_H
#include <stdio.h>
#include <string.h>
extern void GE_DebugInput(FILE *out);
static DWORD ff_trace_last_ms;
static unsigned long ff_trace_raw_events, ff_trace_polls, ff_trace_injected;
static unsigned long ff_trace_accepted[4];
static unsigned int ff_trace_last_device;
static int ff_trace_last_dx[4], ff_trace_last_dy[4];
static char ff_trace_module[MAX_PATH];
static char ff_trace_path[MAX_PATH];

static void FFTraceStart(void)
{
#ifndef MOUSE_INJECTOR_DIAGNOSTICS
    return;
#endif
    HMODULE module = NULL;
    DWORD length;
    char *end;
    ff_trace_last_ms = GetTickCount() - 1000U;
    ff_trace_raw_events = ff_trace_polls = ff_trace_injected = 0;
    memset(ff_trace_accepted, 0, sizeof(ff_trace_accepted));
    memset(ff_trace_last_dx, 0, sizeof(ff_trace_last_dx));
    memset(ff_trace_last_dy, 0, sizeof(ff_trace_last_dy));
    ff_trace_module[0] = ff_trace_path[0] = 0;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&ff_trace_last_ms, &module)) return;
    length = GetModuleFileNameA(module, ff_trace_module, MAX_PATH);
    if(!length || length >= MAX_PATH) { ff_trace_module[0] = 0; return; }
    strcpy(ff_trace_path, ff_trace_module);
    end = strrchr(ff_trace_path, '\\');
    if(!end || (size_t)(end + 1 - ff_trace_path) + sizeof("GEPD-FreeFly.log") > MAX_PATH) {
        ff_trace_path[0] = 0; return;
    }
    strcpy(end + 1, "GEPD-FreeFly.log");
}

static void FFTraceRaw(const ManyMouseEvent *event)
{
    if(event->type == MANYMOUSE_EVENT_RELMOTION) {
        ++ff_trace_raw_events;
        ff_trace_last_device = event->device;
    }
}

static void FFTraceFrame(int valid, int injected)
{
#ifndef MOUSE_INJECTOR_DIAGNOSTICS
    (void)valid; (void)injected; return;
#endif
    DWORD now = GetTickCount();
    FILE *out;
    int player;
    ++ff_trace_polls;
    if(injected) ++ff_trace_injected;
    for(player = 0; player < 4; ++player) {
        if(DEVICE[player].XPOS || DEVICE[player].YPOS) {
            ++ff_trace_accepted[player];
            ff_trace_last_dx[player] = DEVICE[player].XPOS;
            ff_trace_last_dy[player] = DEVICE[player].YPOS;
        }
    }
    if((DWORD)(now - ff_trace_last_ms) < 1000U) return;
    ff_trace_last_ms = now;
    if(!ff_trace_path[0]) return;
    out = fopen(ff_trace_path, "w");
    if(!out) {
        DWORD length = GetTempPathA(MAX_PATH, ff_trace_path);
        if(!length || length + sizeof("GEPD-FreeFly.log") > MAX_PATH) {
            ff_trace_path[0] = 0; return;
        }
        strcpy(ff_trace_path + length, "GEPD-FreeFly.log");
        out = fopen(ff_trace_path, "w");
        if(!out) return;
    }
    fprintf(out, "1964GEPD FreeFly-F3 input diagnostic\nLoaded DLL: %s\n"
        "Capture=%d WindowActive=%d ConfigDialog=%d GameValid=%d\n"
        "CaptureToggleVirtualKey=0x%02X SingleActiveProfile=%d\n"
        "Polls=%lu InjectedPolls=%lu RawMotionEvents=%lu LastMotionDevice=%u\n",
        ff_trace_module, mousetoggle, windowactive, configdialogopen, valid,
        mousetogglekey, !!(ONLY1PLAYERACTIVE), ff_trace_polls, ff_trace_injected,
        ff_trace_raw_events, ff_trace_last_device);
    for(player = 0; player < 4; ++player)
        fprintf(out, "P%d: Config=%d MouseDevice=%d KeyboardDevice=%d Sensitivity=%d "
            "AcceptedMotionPolls=%lu LastMouseDelta=%d,%d\n", player + 1,
            PROFILE[player].SETTINGS[CONFIG], PROFILE[player].SETTINGS[MOUSE],
            PROFILE[player].SETTINGS[KEYBOARD], PROFILE[player].SETTINGS[SENSITIVITY],
            ff_trace_accepted[player], ff_trace_last_dx[player], ff_trace_last_dy[player]);
    GE_DebugInput(out);
    fclose(out);
}
#endif
