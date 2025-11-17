#pragma once

bool cfg_init(const char *filename);
float cfg_get_float(const char *key, float fallback);
int cfg_get_int(const char *key, int fallback);
