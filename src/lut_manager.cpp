#include "lut_manager.h"
#include <math.h>

// 2D-LUTs for mix combinations
static unsigned char lut_blend_5b[32][32];
static unsigned char lut_blend_6b[64][64];

// Linear->sRGB conversion table
unsigned char lut_fwd_5b[32];
unsigned char lut_fwd_6b[64];

// sRGB->Linear conversion table
unsigned char lut_rev_5b[32];
unsigned char lut_rev_6b[64];

static inline float clampf(float x, float lo, float hi) {
    return fminf(fmaxf(x, lo), hi);
}

static void build_lut(unsigned char *lut, int dim, float gamma, float ratio) {
    gamma = fmaxf(1.0, gamma);         // gamma <= 1 == linear blending
    ratio = clampf(ratio, 0.5f, 1.0f); // prio for last frame data

    const float irate = 1.0f - ratio;
    const float igamma = 1.0f / gamma;
    const float maxvalue = (float)(dim - 1);

    unsigned char *dst_fwd, *dst_rev;
    if (dim == 32) {
        dst_fwd = lut_fwd_5b;
        dst_rev = lut_rev_5b;
    } else { // dim == 64
        dst_fwd = lut_fwd_6b;
        dst_rev = lut_rev_6b;
    }

    // generating sRGB colorspace conversion tables
    for (int i = 0; i < dim; i++) {
        const float component = (float)i / maxvalue;

        // building Linear->sRGB conversion table
        float v_fwd = component <= 0.0031308f ? (12.92f * component) * maxvalue
                                              : (1.055f * powf(component, igamma) - 0.055f) * maxvalue;
        dst_fwd[i] = (unsigned char)(clampf(v_fwd, 0.0f, maxvalue) + 0.5f);

        // building sRGB->Linear conversion table
        float v_rev = component <= 0.04045f ? (component / 12.92f) * maxvalue
                                            : powf((component + 0.055f) / 1.055f, gamma) * maxvalue;
        dst_rev[i] = (unsigned char)(clampf(v_rev, 0.0f, maxvalue) + 0.5f);
    }

    // building 2D-LUT for possible combinations
    for (int i = 0; i < dim; i++) {
        unsigned char *row = lut + i * dim;
        for (int j = 0; j < dim; j++) {
            row[j] = dst_fwd[(unsigned)((dst_rev[i] * irate) + (dst_rev[j] * ratio) + 0.5f)];
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
