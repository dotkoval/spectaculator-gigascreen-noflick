#include "lut_manager.h"
#include <math.h>

static unsigned char lut_blend_5b[32][32];
static unsigned char lut_blend_6b[64][64];

static inline float clampf(float x, float lo, float hi) {
    return fminf(fmaxf(x, lo), hi);
}

static void build_lut(unsigned char *lut, int dim, float gamma, float ratio) {
    unsigned char lut_fwd[64];
    unsigned char lut_rev[64];

    gamma = fmaxf(1.0, gamma);         // gamma <= 1 == linear blending
    ratio = clampf(ratio, 0.5f, 1.0f); // prio for last frame data

    const float irate = 1.0f - ratio;
    const float igamma = 1.0f / gamma;
    const float maxvalue = (float)(dim - 1);

    // generating sRGB colorspace conversion tables
    for (int i = 0; i < dim; i++) {
        const float component = (float)i / maxvalue;

        // building Linear->sRGB conversion table
        float v_fwd = 1.055f * (powf(component, igamma) - 0.055f) * maxvalue;
        lut_fwd[i] = (unsigned char)(clampf(v_fwd, 0.0f, maxvalue) + 0.5f);

        // building sRGB->Linear conversion table
        float v_rev = powf((component + 0.055f) / 1.055f, gamma) * maxvalue;
        lut_rev[i] = (unsigned char)(clampf(v_rev, 0.0f, maxvalue) + 0.5f);
    }

    // building LUT for possible combinations
    for (int i = 0; i < dim; i++) {
        unsigned char *row = lut + i * dim;
        for (int j = 0; j < dim; j++) {
            row[j] = lut_fwd[(unsigned)((lut_rev[i] * irate) + (lut_rev[j] * ratio) + 0.5f)];
        }
    }
}

lut5_ptr lutmgr_init_5b(float gamma, float ratio) {
    build_lut(&lut_blend_5b[0][0], 32, gamma, ratio);
    return (lut5_ptr)lut_blend_5b;
}

lut6_ptr lutmgr_init_6b(float gamma, float ratio) {
    build_lut(&lut_blend_6b[0][0], 64, gamma, ratio);
    return (lut6_ptr)lut_blend_6b;
}
