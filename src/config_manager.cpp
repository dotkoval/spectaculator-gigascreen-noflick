//------------------------------------------------------------------------------
// Configuration Manager for Gigascreen Render Plugin
//
// Loads configuration values from a text file located in the same directory
// as the plugin DLL. Parses simple key=value pairs and provides access to
// stored entries.
//
// If the file does not exist, it is created automatically with defaults:
// mode=1
// gamma=2.2
// ratio=0.5
// fullbright=1
//
// Check README.md for more details.
//------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include "config_manager.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define CFG_MAX_ENTRIES 32
#define CFG_KEY_MAX 64
#define CFG_VAL_MAX 128

typedef struct {
    char key[CFG_KEY_MAX];
    char val[CFG_VAL_MAX];
} cfg_entry_t;

static char s_path[MAX_PATH] = {0};
static cfg_entry_t s_entries[CFG_MAX_ENTRIES];
static int s_count = 0;
static bool s_inited = false;

// - Helpers -------------------------------------------------------------------

static void trim(char *s) {
    char *p = s;
    while (*p && isspace((unsigned char)*p))
        ++p;
    if (p != s)
        memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1]))
        s[--n] = 0;
}

static void tolower_str(char *s) {
    for (; *s; ++s)
        *s = (char)tolower((unsigned char)*s);
}

static void get_dll_dir(char out_dir[MAX_PATH]) {
    out_dir[0] = 0;
    HMODULE self = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)&get_dll_dir, &self)) {
        char full[MAX_PATH] = {0};
        if (GetModuleFileNameA(self, full, MAX_PATH)) {
            size_t n = strlen(full);
            while (n && full[n - 1] != '\\' && full[n - 1] != '/')
                --n;
            memcpy(out_dir, full, n);
            out_dir[n] = 0;
        }
    }
}

static void join_path(char out[MAX_PATH], const char *dir, const char *name) {
    if (!dir || !*dir) {
        strncpy(out, name, MAX_PATH - 1);
        out[MAX_PATH - 1] = 0;
        return;
    }
    _snprintf(out, MAX_PATH, "%s\\%s", dir, name);
}

static bool write_text_file(const char *path, const char *text) {
    FILE *f = fopen(path, "wb");
    if (!f)
        return false;
    if (text)
        fputs(text, f);
    fclose(f);
    return true;
}

static void comma_to_dot(char *s) {
    for (; *s; ++s)
        if (*s == ',')
            *s = '.';
}

// - Parser --------------------------------------------------------------------

static void cfg_clear(void) {
    s_count = 0;
}

static void cfg_add(const char *key, const char *val) {
    if (!key || !*key || !val)
        return;
    if (s_count >= CFG_MAX_ENTRIES)
        return;

    for (int i = 0; i < s_count; ++i) {
        if (_stricmp(s_entries[i].key, key) == 0) {
            strncpy(s_entries[i].val, val, CFG_VAL_MAX - 1);
            s_entries[i].val[CFG_VAL_MAX - 1] = 0;
            return;
        }
    }

    strncpy(s_entries[s_count].key, key, CFG_KEY_MAX - 1);
    s_entries[s_count].key[CFG_KEY_MAX - 1] = 0;
    strncpy(s_entries[s_count].val, val, CFG_VAL_MAX - 1);
    s_entries[s_count].val[CFG_VAL_MAX - 1] = 0;
    ++s_count;
}

static bool cfg_load_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;

    cfg_clear();

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        trim(line);
        if (!*line)
            continue;
        if (line[0] == '#' || line[0] == ';')
            continue;

        char *eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = 0;
        char *key = line;
        char *val = eq + 1;
        trim(key);
        trim(val);
        tolower_str(key);

        cfg_add(key, val);
    }

    fclose(f);
    return true;
}

static const cfg_entry_t *find_entry(const char *key) {
    if (!key)
        return NULL;
    for (int i = 0; i < s_count; ++i) {
        if (_stricmp(s_entries[i].key, key) == 0)
            return &s_entries[i];
    }
    return NULL;
}

// - Public API ----------------------------------------------------------------

bool cfg_init(const char *filename) {
    char dir[MAX_PATH] = {0};
    get_dll_dir(dir);
    join_path(s_path, dir, filename ? filename : "gigascreen.cfg");

    FILE *f = fopen(s_path, "rb");
    if (!f) {
        const char *defaults = "mode=2\ngamma=2.2\nratio=0.5\nfullbright=0\n";
        if (!write_text_file(s_path, defaults))
            return false;
    } else {
        fclose(f);
    }

    if (!cfg_load_file(s_path))
        return false;

    s_inited = true;
    return true;
}

float cfg_get_float(const char *key, float fallback) {
    if (!s_inited)
        return fallback;

    const cfg_entry_t *e = find_entry(key);
    if (!e)
        return fallback;

    char buf[CFG_VAL_MAX];
    strncpy(buf, e->val, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    comma_to_dot(buf);
    char *end = NULL;
    double v = strtod(buf, &end);
    if (end == buf)
        return fallback;
    return (float)v;
}

int cfg_get_int(const char *key, int fallback) {
    if (!s_inited)
        return fallback;

    const cfg_entry_t *e = find_entry(key);
    if (!e)
        return fallback;

    char buf[CFG_VAL_MAX];
    strncpy(buf, e->val, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char *end = NULL;
    long v = strtol(buf, &end, 10);
    if (end == buf)
        return fallback;
    return (int)v;
}
