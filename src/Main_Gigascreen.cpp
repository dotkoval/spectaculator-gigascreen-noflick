//---------------------------------------------------------------------------------------------------------------------------
// Gigascreen No-Flick Render Plugin for Spectaculator
// by oleksandr ".koval" kovalchuk
//---------------------------------------------------------------------------------------------------------------------------

// Main_Gigascreen.cpp
// Simple temporal-blend render plugin (2x) for Spectaculator.
// Blends previous and current 16bpp frames (RGB565) and outputs
// a 2x image. Exports are provided by RPI.h.
//
// Build (Win32/x86):
//   Debug:   cl /LD /Zi /EHsc /MDd /DWIN32 /D_WINDOWS /D_DEBUG /DRENDERPLUGS_EXPORTS Main_Gigascreen.cpp /link /OUT:Gigascreen_d.rpi
//   Release: cl /LD /O2 /EHsc /MD  /DWIN32 /D_WINDOWS /DNDEBUG /DRENDERPLUGS_EXPORTS Main_Gigascreen.cpp /link /OUT:Gigascreen.rpi
//
// Notes:
// - Architecture must be Win32 (x86). Call vcvars32.bat before building.
// - No .def needed: RPI.h already does __declspec(dllexport) for both symbols.
// - My guess is that Spectaculator operates in RGB565 colorspace only, so no need to support other formats
// - As RenderPlugins does not support any configuration - options are cooked in separated binaries

#ifndef GAMMA
#   define GAMMA gamma25
#endif

#ifdef RETRO_VIBES
#   define NOFLICK noflick80
#   define NOFLICK_TITLE "80%"
#else
#   define NOFLICK noflick100
#   define NOFLICK_TITLE "100%"
#endif

#include "RPI.h"
#include <vector>
#include <cstring>

static std::vector<WORD> s_prev;
static bool s_havePrev = false;
static unsigned s_w = 0, s_h = 0;

// Here is some magic with generating path to LUT for different GAMMA and NOFLICK
#define STR2(x) #x
#define STR(x) STR2(x)

#if GAMMA == 25
#   define GDIR gamma25
#   define GAMMA_TITLE "G2.5"
#elif GAMMA == 24
#   define GDIR gamma24
#   define GAMMA_TITLE "G2.4"
#elif GAMMA == 22
#   define GDIR gamma22
#   define GAMMA_TITLE "G2.2"
#elif GAMMA == 20
#   define GDIR gamma20
#   define GAMMA_TITLE "G2.0"
#elif GAMMA == 18
#   define GDIR gamma18
#   define GAMMA_TITLE "G1.8"
#else
#   define GDIR linear
#   define GAMMA_TITLE "Linear"
#endif

#define LUT_B5 STR(luts/##GDIR##/##NOFLICK##/lut_blend_5b.h)
#define LUT_B6 STR(luts/##GDIR##/##NOFLICK##/lut_blend_6b.h)

#include LUT_B5
#include LUT_B6

extern "C" RENDER_PLUGIN_INFO* RenderPluginGetInfo(void)
{
    // Max 60 chars, follow the style used by sample plugins.
    rpi_strcpy(&MyRPI.Name[0], "Gigascreen No-Flick " NOFLICK_TITLE " " GAMMA_TITLE " (.koval)");

    // 16bpp input format (RGB565) + fixed 2x output scale.
    MyRPI.Flags = RPI_VERSION | RPI_565_SUPP | RPI_OUT_SCL2;
    return &MyRPI;
}

extern "C" void RenderPluginOutput(RENDER_PLUGIN_OUTP* rpo)
{
    const unsigned w  = rpo->SrcW;
    const unsigned h  = rpo->SrcH;
    const unsigned sp = rpo->SrcPitch / 2; // WORDs per source row (16 bpp)
    const unsigned dp = rpo->DstPitch / 2; // WORDs per dest   row (16 bpp)

    // (Re)allocate previous-frame buffer on size change.
    if (w != s_w || h != s_h) {
        s_prev.assign(w * h, 0);
        s_havePrev = false;
        s_w = w; s_h = h;
    }

    // Ensure destination can hold a 2x image.
    if (!((w * 2) <= rpo->DstW && (h * 2) <= rpo->DstH)) {
        rpo->OutW = rpo->OutH = 0;
        return;
    }

    const WORD* src = (const WORD*)rpo->SrcPtr;
    WORD*       dst = (WORD*)rpo->DstPtr;

    if (!s_havePrev) {
        // First frame: pass-through 2x, also seed the previous buffer.
        for (unsigned y = 0; y < h; ++y) {
            const WORD* srow = src + y * sp;
            WORD*       drow0 = dst + (y * 2) * dp;
            WORD*       drow1 = drow0 + dp;

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
            const WORD* srow = src + y * sp;
            WORD*       drow0 = dst + (y * 2) * dp;
            WORD*       drow1 = drow0 + dp;
            WORD*       prow  = &s_prev[y * w];

            for (unsigned x = 0; x < w; ++x) {

                unsigned pr = (prow[x] >> 11) & 0x1F;
                unsigned pg = (prow[x] >> 5) & 0x3F;
                unsigned pb = prow[x] & 0x1F;

                unsigned sr = (srow[x] >> 11) & 0x1F;
                unsigned sg = (srow[x] >> 5) & 0x3F;
                unsigned sb = srow[x] & 0x1F;

                unsigned cr = lut_blend_5b[pr][sr] << 11;
                unsigned cg = lut_blend_6b[pg][sg] << 5;
                unsigned cb = lut_blend_5b[pb][sb];

                WORD out = cr | cg | cb;

                drow0[x * 2 + 0] = out;
                drow0[x * 2 + 1] = out;
                prow[x] = srow[x]; // update previous
            }
            std::memcpy(drow1, drow0, (w * 2) * sizeof(WORD));
        }
    }

    // Report actual output size.
    rpo->OutW = w * 2;
    rpo->OutH = h * 2;
}
