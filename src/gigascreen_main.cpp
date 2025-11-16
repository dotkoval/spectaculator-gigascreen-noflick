//------------------------------------------------------------------------------
// Gigascreen No-Flick Render Plugin for Spectaculator
// by oleksandr ".koval" kovalchuk
//------------------------------------------------------------------------------
//
// Simple temporal-blend render plugin (2x) for Spectaculator.
// Blends previous and current 16bpp frames (RGB565) and outputs
// a 2x image. Exports are provided by rpi.h.
//
// Platform support:
// - Windows (Win32/x86) only.
//   This code uses WinAPI (windows.h, DllMain) and is not portable as-is
//   to other platforms.
// - macOS builds are NOT supported, as I currently have no ability to build
//   or test the plugin on macOS.
//
// Build (Win32/x86):
//   See build.cmd (Windows) for full build commands.
//
// Notes:
// - Architecture must be Win32 (x86). Call vcvars32.bat before building.
// - No .def needed: rpi.h already does __declspec(dllexport) for both symbols.
// - My guess is that Spectaculator operates in RGB565 colorspace only, so no
//   need to support other formats.
// - As RenderPlugins does not support any configuration - options are cooked in
//   separated binaries.
//------------------------------------------------------------------------------

#include "config_manager.h"
#include "lut_manager.h"
#include "rpi.h"
#include <cstring>
#include <vector>
#include <windows.h>

#ifndef PLUGIN_TITLE
#define PLUGIN_TITLE "Gigascreen No-Flick (.koval)"
#endif

static std::vector<WORD> s_prev;
static bool s_havePrev = false;
static unsigned s_w = 0, s_h = 0;
static int mode = 1;

static lut5_ptr lut_blend_5b = nullptr;
static lut6_ptr lut_blend_6b = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Initialize configuration file
        cfg_init("gigascreen.cfg");

        // Read gamma and blending ratio values from the configuration
        float gamma = cfg_get_float("gamma", 2.4);
        float ratio = cfg_get_float("ratio", 0.5);
        mode = cfg_get_int("mode", mode);

        // Initialize default gamma lookup tables (LUTs)
        lut_blend_5b = lutmgr_init_5b(gamma, ratio);
        lut_blend_6b = lutmgr_init_6b(gamma, ratio);
    }
    return TRUE;
}

extern "C" RENDER_PLUGIN_INFO *RenderPluginGetInfo(void) {
    // Max 60 chars, follow the style used by sample plugins.
    rpi_strcpy(&MyRPI.Name[0], PLUGIN_TITLE);
    // 16bpp input format (RGB565) + fixed 2x output scale.
    MyRPI.Flags = RPI_VERSION | RPI_565_SUPP | RPI_OUT_SCL2;
    return &MyRPI;
}

extern "C" void RenderPluginOutput(RENDER_PLUGIN_OUTP *rpo) {
    const unsigned w = rpo->SrcW;
    const unsigned h = rpo->SrcH;
    const unsigned sp = rpo->SrcPitch / 2; // WORDs per source row (16 bpp)
    const unsigned dp = rpo->DstPitch / 2; // WORDs per dest   row (16 bpp)

    // (Re)allocate previous-frame buffer on size change.
    if (w != s_w || h != s_h) {
        s_prev.assign(w * h, 0);
        s_havePrev = false;
        s_w = w;
        s_h = h;
    }

    // Ensure destination can hold a 2x image.
    if (!((w * 2) <= rpo->DstW && (h * 2) <= rpo->DstH)) {
        rpo->OutW = rpo->OutH = 0;
        return;
    }

    const WORD *src = (const WORD *)rpo->SrcPtr;
    WORD *dst = (WORD *)rpo->DstPtr;

    if (!s_havePrev) {
        // First frame: pass-through 2x, also seed the previous buffer.
        for (unsigned y = 0; y < h; ++y) {
            const WORD *srow = src + y * sp;
            WORD *drow0 = dst + (y * 2) * dp;
            WORD *drow1 = drow0 + dp;

            for (unsigned x = 0; x < w; ++x) {
                WORD px = srow[x];
                drow0[x * 2 + 0] = px;
                drow0[x * 2 + 1] = px;
                s_prev[y * w + x] = px;
            }
            std::memcpy(drow1, drow0, (w * 2) * sizeof(WORD));
        }
        s_havePrev = true;
    } else {
        // Blend prev+curr per pixel, then 2x replicate.
        for (unsigned y = 0; y < h; ++y) {
            const WORD *srow = src + y * sp;
            WORD *drow0 = dst + (y * 2) * dp;
            WORD *drow1 = drow0 + dp;
            WORD *prow = &s_prev[y * w];

            for (unsigned x = 0; x < w; ++x) {

                // Mode 0: antiflicker is disabled
                WORD out = srow[x];

                // Mode 1: antiflicker is enabled (Gigascreen mode)
                if (mode == 1) {
                    unsigned pr = (prow[x] >> 11) & 0x1F;
                    unsigned pg = (prow[x] >> 5) & 0x3F;
                    unsigned pb = prow[x] & 0x1F;

                    unsigned sr = (srow[x] >> 11) & 0x1F;
                    unsigned sg = (srow[x] >> 5) & 0x3F;
                    unsigned sb = srow[x] & 0x1F;

                    unsigned cr = lut_blend_5b[pr][sr] << 11;
                    unsigned cg = lut_blend_6b[pg][sg] << 5;
                    unsigned cb = lut_blend_5b[pb][sb];

                    out = cr | cg | cb;
                }

                drow0[x * 2 + 0] = out;
                drow0[x * 2 + 1] = out;

                // update previous row
                prow[x] = srow[x];
            }
            // copy every full row
            std::memcpy(drow1, drow0, (w * 2) * sizeof(WORD));
        }
    }

    // Report actual output size.
    rpo->OutW = w * 2;
    rpo->OutH = h * 2;
}
