#ifndef WINDOWS_COMPAT_H
#define WINDOWS_COMPAT_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <wchar.h>
#define PATH_MAX 32768
#define strdup _strdup
#define strcasecmp _stricmp

static wchar_t *win_wide(const char *s) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    wchar_t *w = n ? malloc((size_t)n * sizeof(wchar_t)) : NULL;
    if (w) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, w, n);
    return w;
}
static char *win_utf8(const wchar_t *w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    char *s = n ? malloc(n) : NULL;
    if (s) WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL);
    return s;
}
static FILE *win_fopen(const char *path, const char *mode) {
    wchar_t *p = win_wide(path), *m = win_wide(mode);
    FILE *f = p && m ? _wfopen(p, m) : NULL;
    free(p); free(m); return f;
}
static char *win_fullpath(const char *path, char *out) {
    wchar_t *p = win_wide(path), w[PATH_MAX];
    DWORD n = p ? GetFullPathNameW(p, PATH_MAX, w, NULL) : 0;
    free(p);
    if (!n || n >= PATH_MAX) return NULL;
    char *s = win_utf8(w);
    if (!s || strlen(s) >= PATH_MAX) { free(s); return NULL; }
    strcpy(out, s); free(s);
    for (char *c = out; *c; ++c) if (*c == '\\') *c = '/';
    size_t len = strlen(out);
    while (len > 3 && out[len-1] == '/') out[--len] = 0;
    return out;
}
static char *win_getcwd(char *out, size_t size) {
    char full[PATH_MAX];
    if (!win_fullpath(".", full) || strlen(full) >= size) return NULL;
    strcpy(out, full); return out;
}
static int win_mkdir(const char *path, int mode) {
    (void)mode;
    wchar_t *w = win_wide(path);
    if (!w) return -1;
    int result = _wmkdir(w); free(w); return result;
}
static DWORD win_attributes(const char *path) {
    wchar_t *w = win_wide(path);
    DWORD result = w ? GetFileAttributesW(w) : INVALID_FILE_ATTRIBUTES;
    free(w); return result;
}
#define fopen win_fopen
#define realpath win_fullpath
#define getcwd win_getcwd
#define mkdir win_mkdir
#define localtime_r(t, result) localtime_s(result, t)
#endif
