#pragma once

typedef unsigned char (*lut5_ptr)[32];
typedef unsigned char (*lut6_ptr)[64];

lut5_ptr lutmgr_init_5b(float gamma, float ratio);
lut6_ptr lutmgr_init_6b(float gamma, float ratio);
