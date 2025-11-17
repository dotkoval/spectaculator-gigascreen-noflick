#pragma once

// Linear->sRGB conversion table
extern unsigned char lut_fwd_5b[];
extern unsigned char lut_fwd_6b[];

// sRGB->Linear conversion table
extern unsigned char lut_rev_5b[];
extern unsigned char lut_rev_6b[];

// 2D-LUTs for mix combinations
typedef unsigned char (*lut5_ptr)[32];
typedef unsigned char (*lut6_ptr)[64];

lut5_ptr lutmgr_init_5b(float gamma, float ratio);
lut6_ptr lutmgr_init_6b(float gamma, float ratio);
