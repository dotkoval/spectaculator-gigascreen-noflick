//------------------------------------------------------------------------------
// Gigascreen No-Flick Render Plugin for Spectaculator (and ZXSpin)
// by oleksandr ".koval" kovalchuk
//------------------------------------------------------------------------------
//
// Simple temporal-blend render plugin (2x) for Spectaculator and ZXSpin.
// Blends 16bpp frames (RGB565) using precomputed LUTs and outputs
// a 2x image. Exports are provided by rpi.h.
// Supports Gigascreen (2-frame) and experimental 3Color (3-frame) modes.
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
// - As RenderPlugins do not expose runtime configuration, options are
//   controlled via a text config file (gigascreen.cfg) placed next to the DLL.
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

// Default configuration values
#define DEFAULT_MODE 2
#define DEFAULT_GAMMA 2.2
#define DEFAULT_RATIO 0.5
#define DEFAULT_FULLBRIGHT 0
#define DEFAULT_MOTION_DETECTION 0

// Keep last N-frames for 3Color mode evaluation
#define FRAME_HISTORY 5

static std::vector<WORD> frame_history; // ring buffer for frame history
static unsigned frame_size = 0;
static unsigned last_frame_idx = 0;
static unsigned s_w = 0;
static unsigned s_h = 0;
static bool s_havePrev = false;
static int mode = DEFAULT_MODE;
static int fullbright = DEFAULT_FULLBRIGHT;
static int motion_check = DEFAULT_MOTION_DETECTION;

static lut5_ptr lut_blend_5b = nullptr;
static lut6_ptr lut_blend_6b = nullptr;

// - Helpers -------------------------------------------------------------------
static bool prev_shift_tab = false;

static inline bool key_down(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// Returns true only on the transition "not pressed" -> "pressed" for Shift+Tab.
bool shift_tab_pressed_once() {
    bool now = (key_down(VK_TAB) && (key_down(VK_LSHIFT) /* || key_down(VK_RSHIFT) */));

    bool triggered = (!prev_shift_tab && now);
    prev_shift_tab = now;
    return triggered;
}

// - Plugin init on DLL attachment ---------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Initialize configuration file
        cfg_init("gigascreen.cfg");

        // Read gamma and blending ratio values from the configuration
        float gamma = cfg_get_float("gamma", DEFAULT_GAMMA);
        float ratio = cfg_get_float("ratio", DEFAULT_RATIO);
        mode = cfg_get_int("mode", mode);
        fullbright = cfg_get_int("fullbright", fullbright);
        motion_check = cfg_get_int("motion_check", motion_check);

        // Initialize gamma lookup tables (LUTs) according to configuration
        lut_blend_5b = lutmgr_init_5b(gamma, ratio);
        lut_blend_6b = lutmgr_init_6b(gamma, ratio);
    }
    return TRUE;
}

// - Blending functions --------------------------------------------------------
// Gigascreen blending via LUTs
static inline unsigned gigascreen_blend(unsigned p0, unsigned p1) {
    // Extract RGB pixel components for current frame (5-6-5 packed format)
    unsigned frame0_r = (p0 >> 11) & 0x1F;
    unsigned frame0_g = (p0 >> 5) & 0x3F;
    unsigned frame0_b = p0 & 0x1F;

    // Extract RGB pixel components for previous frame (5-6-5 packed format)
    unsigned frame1_r = (p1 >> 11) & 0x1F;
    unsigned frame1_g = (p1 >> 5) & 0x3F;
    unsigned frame1_b = p1 & 0x1F;

    // Look up precomputed blended components in encoded (5/6-bit) space
    unsigned r = lut_blend_5b[frame1_r][frame0_r] << 11;
    unsigned g = lut_blend_6b[frame1_g][frame0_g] << 5;
    unsigned b = lut_blend_5b[frame1_b][frame0_b];

    return r | g | b;
}

// 3Color blending in linear light using LUTs
static inline unsigned tricolor_blend(unsigned p0, unsigned p1, unsigned p2) {
    // Decode RGB components from 5-6-5 encoded space to linear colorspace (sRGB -> linear)
    unsigned frame0_r = lut_rev_5b[(p0 >> 11) & 0x1F];
    unsigned frame0_g = lut_rev_6b[(p0 >> 5) & 0x3F];
    unsigned frame0_b = lut_rev_5b[p0 & 0x1F];

    unsigned frame1_r = lut_rev_5b[(p1 >> 11) & 0x1F];
    unsigned frame1_g = lut_rev_6b[(p1 >> 5) & 0x3F];
    unsigned frame1_b = lut_rev_5b[p1 & 0x1F];

    unsigned frame2_r = lut_rev_5b[(p2 >> 11) & 0x1F];
    unsigned frame2_g = lut_rev_6b[(p2 >> 5) & 0x3F];
    unsigned frame2_b = lut_rev_5b[p2 & 0x1F];

    // Encode averaged linear components back to 5-6-5 encoded space (linear -> sRGB)
    unsigned r = lut_fwd_5b[(frame0_r + frame1_r + frame2_r) / 3] << 11;
    unsigned g = lut_fwd_6b[(frame0_g + frame1_g + frame2_g) / 3] << 5;
    unsigned b = lut_fwd_5b[(frame0_b + frame1_b + frame2_b) / 3];

    return r | g | b;
}

// Check if RGB565 pixel has more than one color component
static inline bool rgb565_has_multi_component(unsigned int c) {

    // Components
    unsigned r = c & 0b1111100000000000; // Red
    unsigned g = c & 0b0000011111100000; // Green
    unsigned b = c & 0b0000000000011111; // Blue

    // More than one non-zero component?
    return ((r != 0) + (g != 0) + (b != 0)) > 1;
}

// - Plugin Info ---------------------------------------------------------------
extern "C" RENDER_PLUGIN_INFO *RenderPluginGetInfo(void) {
    // Max 60 chars, follow the style used by sample plugins.
    rpi_strcpy(&MyRPI.Name[0], PLUGIN_TITLE);
    // 16bpp input format (RGB565) + fixed 2x output scale.
    MyRPI.Flags = RPI_VERSION | RPI_565_SUPP | RPI_OUT_SCL2;
    return &MyRPI;
}

// - MAIN Plugin routine -------------------------------------------------------
extern "C" void RenderPluginOutput(RENDER_PLUGIN_OUTP *rpo) {
    const unsigned w = rpo->SrcW;
    const unsigned h = rpo->SrcH;
    const unsigned sp = rpo->SrcPitch / 2; // WORDs per source row (16 bpp)
    const unsigned dp = rpo->DstPitch / 2; // WORDs per dest   row (16 bpp)

    // (Re)allocate frame history buffer on size change.
    if (w != s_w || h != s_h) {
        frame_size = w * h;
        frame_history.assign(frame_size * FRAME_HISTORY, 0);
        s_havePrev = false; // history not initialized yet
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
        // First frame: pass-through 2x, also seed the history ring buffer.
        for (unsigned y = 0; y < h; ++y) {
            const WORD *srow = src + y * sp;
            WORD *drow0 = dst + (y * 2) * dp;
            WORD *drow1 = drow0 + dp;

            for (unsigned x = 0; x < w; ++x) {
                WORD px = srow[x];
                drow0[x * 2 + 0] = px;
                drow0[x * 2 + 1] = px;
                // Initialize all history slots with the current frame
                frame_history[y * w + x + frame_size * 0] = px;
                frame_history[y * w + x + frame_size * 1] = px;
                frame_history[y * w + x + frame_size * 2] = px;
                frame_history[y * w + x + frame_size * 3] = px;
                frame_history[y * w + x + frame_size * 4] = px;
            }
            std::memcpy(drow1, drow0, (w * 2) * sizeof(WORD));
        }
        s_havePrev = true;
    } else {
        if (shift_tab_pressed_once()) {
            // rotate Mode
            mode = (mode + 1) % 3;
        }

        // Compute indices into the frame history ring buffer (most recent first)
        int idx_p0 = last_frame_idx;                       // N-1 (most recent stored)
        int idx_p1 = (last_frame_idx + 1) % FRAME_HISTORY; // N-2
        int idx_p2 = (last_frame_idx + 2) % FRAME_HISTORY; // N-3
        int idx_p3 = (last_frame_idx + 3) % FRAME_HISTORY; // N-4
        int idx_p4 = (last_frame_idx + 4) % FRAME_HISTORY; // N-5

        // Advance write position for the next frame to be stored
        last_frame_idx = (last_frame_idx + FRAME_HISTORY - 1) % FRAME_HISTORY;

        // Blend per-pixel according to the current mode, then 2x replicate.
        for (unsigned y = 0; y < h; ++y) {
            const WORD *src_row = src + y * sp;
            WORD *dst_row0 = dst + (y * 2) * dp;
            WORD *dst_row1 = dst_row0 + dp;
            WORD *prev_frame0_row = &frame_history[y * w + frame_size * idx_p0];
            WORD *prev_frame1_row = &frame_history[y * w + frame_size * idx_p1];
            WORD *prev_frame2_row = &frame_history[y * w + frame_size * idx_p2];
            WORD *prev_frame3_row = &frame_history[y * w + frame_size * idx_p3];
            WORD *prev_frame4_row = &frame_history[y * w + frame_size * idx_p4];

            for (unsigned x = 0; x < w; ++x) {

                WORD p0 = src_row[x];         // pixel at frame N-0 (current)
                WORD p1 = prev_frame0_row[x]; // pixel at frame N-1
                WORD p2 = prev_frame1_row[x]; // pixel at frame N-2
                WORD p3 = prev_frame2_row[x]; // pixel at frame N-3
                WORD p4 = prev_frame3_row[x]; // pixel at frame N-4
                WORD p5 = prev_frame4_row[x]; // pixel at frame N-5

                // Mode 0: antiflicker is disabled (fallback option)
                WORD out = p0;
                bool multi_components;

                switch (mode) {
                // Mode 2: antiflicker is enabled (Gigascreen+3Color)
                case 2:
                    // 3Color simple check
                    multi_components = rgb565_has_multi_component(p0) ||
                                       rgb565_has_multi_component(p1) ||
                                       rgb565_has_multi_component(p2);

                    if (!multi_components && p0 == p3 && p1 == p4 && p2 == p5) {
                        //  Fullbright blending (simple mix, no gamma correction)
                        if (fullbright)
                            out = p0 | p1 | p2;
                        else
                            out = tricolor_blend(p0, p1, p2);
                    } else {
                        // fallback to Gigascreen mode
                        if (!motion_check || (p0 == p2 && p0 != p1 && p1 != p2))
                            out = gigascreen_blend(p0, p1);
                    }
                    break;

                // Mode 1: antiflicker is enabled (Gigascreen only)
                case 1:
                    if (!motion_check || (p0 == p2 && p0 != p1))
                        out = gigascreen_blend(p0, p1);
                    break;
                }

                dst_row0[x * 2 + 0] = out;
                dst_row0[x * 2 + 1] = out;

                // store current pixel in the newest history slot
                prev_frame4_row[x] = p0;
            }
            // copy every full row
            std::memcpy(dst_row1, dst_row0, (w * 2) * sizeof(WORD));
        }
    }

    // Report actual output size.
    rpo->OutW = w * 2;
    rpo->OutH = h * 2;
}
