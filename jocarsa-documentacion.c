#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <strings.h>
#endif
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#ifndef _WIN32
#include <dirent.h>
#endif
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <limits.h>
#include <time.h>
#ifdef _WIN32
#include "windows_compat.h"
#else
#include <pwd.h>
#include <sys/utsname.h>
#endif
#include <sqlite3.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#define EXCLUSION_MARKER ".jocarsa-documentacion-exclude"
typedef struct  {
    char *data;
    size_t len, cap;
}
Buffer;
typedef struct  {
    char **v;
    size_t n, cap;
}
StrVec;

static void buf_append(Buffer *b, const char *s);
static void buf_printf(Buffer *b, const char *fmt, ...);

/* ============================================================
   SHA-256
   Implementación local para no añadir dependencias adicionales.
   ============================================================ */

typedef struct
{
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
}
SHA256Context;

static const uint32_t SHA256_K[64] =
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static uint32_t sha256_rotateright(uint32_t value, uint32_t count)
{
    return (value >> count) | (value << (32 - count));
}

static void sha256_transform(SHA256Context *context, const uint8_t data[])
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f;
    uint32_t g;
    uint32_t h;
    uint32_t i;
    uint32_t j;
    uint32_t t1;
    uint32_t t2;
    uint32_t m[64];

    for (i = 0, j = 0; i < 16; i++, j += 4)
    {
        m[i] = ((uint32_t)data[j] << 24) |
               ((uint32_t)data[j + 1] << 16) |
               ((uint32_t)data[j + 2] << 8) |
               ((uint32_t)data[j + 3]);
    }

    for (; i < 64; i++)
    {
        uint32_t s0;
        uint32_t s1;

        s0 = sha256_rotateright(m[i - 15], 7) ^
             sha256_rotateright(m[i - 15], 18) ^
             (m[i - 15] >> 3);

        s1 = sha256_rotateright(m[i - 2], 17) ^
             sha256_rotateright(m[i - 2], 19) ^
             (m[i - 2] >> 10);

        m[i] = m[i - 16] + s0 + m[i - 7] + s1;
    }

    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    for (i = 0; i < 64; i++)
    {
        uint32_t S1;
        uint32_t choice;
        uint32_t S0;
        uint32_t majority;

        S1 = sha256_rotateright(e, 6) ^
             sha256_rotateright(e, 11) ^
             sha256_rotateright(e, 25);

        choice = (e & f) ^ ((~e) & g);
        t1 = h + S1 + choice + SHA256_K[i] + m[i];

        S0 = sha256_rotateright(a, 2) ^
             sha256_rotateright(a, 13) ^
             sha256_rotateright(a, 22);

        majority = (a & b) ^ (a & c) ^ (b & c);
        t2 = S0 + majority;

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

static void sha256_init(SHA256Context *context)
{
    context->datalen = 0;
    context->bitlen = 0;

    context->state[0] = 0x6a09e667;
    context->state[1] = 0xbb67ae85;
    context->state[2] = 0x3c6ef372;
    context->state[3] = 0xa54ff53a;
    context->state[4] = 0x510e527f;
    context->state[5] = 0x9b05688c;
    context->state[6] = 0x1f83d9ab;
    context->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256Context *context, const uint8_t *data, size_t length)
{
    size_t i;

    for (i = 0; i < length; i++)
    {
        context->data[context->datalen] = data[i];
        context->datalen++;

        if (context->datalen == 64)
        {
            sha256_transform(context, context->data);
            context->bitlen += 512;
            context->datalen = 0;
        }
    }
}

static void sha256_final(SHA256Context *context, uint8_t hash[32])
{
    uint32_t i;

    i = context->datalen;

    if (context->datalen < 56)
    {
        context->data[i++] = 0x80;

        while (i < 56)
        {
            context->data[i++] = 0x00;
        }
    }
    else
    {
        context->data[i++] = 0x80;

        while (i < 64)
        {
            context->data[i++] = 0x00;
        }

        sha256_transform(context, context->data);
        memset(context->data, 0, 56);
    }

    context->bitlen += context->datalen * 8;

    context->data[63] = context->bitlen;
    context->data[62] = context->bitlen >> 8;
    context->data[61] = context->bitlen >> 16;
    context->data[60] = context->bitlen >> 24;
    context->data[59] = context->bitlen >> 32;
    context->data[58] = context->bitlen >> 40;
    context->data[57] = context->bitlen >> 48;
    context->data[56] = context->bitlen >> 56;

    sha256_transform(context, context->data);

    for (i = 0; i < 4; i++)
    {
        hash[i]      = (context->state[0] >> (24 - i * 8)) & 0xff;
        hash[i + 4]  = (context->state[1] >> (24 - i * 8)) & 0xff;
        hash[i + 8]  = (context->state[2] >> (24 - i * 8)) & 0xff;
        hash[i + 12] = (context->state[3] >> (24 - i * 8)) & 0xff;
        hash[i + 16] = (context->state[4] >> (24 - i * 8)) & 0xff;
        hash[i + 20] = (context->state[5] >> (24 - i * 8)) & 0xff;
        hash[i + 24] = (context->state[6] >> (24 - i * 8)) & 0xff;
        hash[i + 28] = (context->state[7] >> (24 - i * 8)) & 0xff;
    }
}

static const char *SECRETO_JOCARSA =
    "JOCARSA-documentacion-2026-7f3a91c8e42b";

static void hmac_sha256(
    const unsigned char *key,
    size_t key_length,
    const unsigned char *data,
    size_t data_length,
    char hexadecimal[65]
)
{
    unsigned char key_block[64];
    unsigned char inner_padding[64];
    unsigned char outer_padding[64];
    unsigned char inner_hash[32];
    unsigned char final_hash[32];
    unsigned char temporary_key[32];
    SHA256Context context;
    size_t i;

    memset(key_block, 0, sizeof(key_block));

    if (key_length > 64)
    {
        sha256_init(&context);
        sha256_update(&context, key, key_length);
        sha256_final(&context, temporary_key);

        memcpy(key_block, temporary_key, 32);
    }
    else
    {
        memcpy(key_block, key, key_length);
    }

    for (i = 0; i < 64; i++)
    {
        inner_padding[i] = key_block[i] ^ 0x36;
        outer_padding[i] = key_block[i] ^ 0x5c;
    }

    sha256_init(&context);
    sha256_update(&context, inner_padding, 64);
    sha256_update(&context, data, data_length);
    sha256_final(&context, inner_hash);

    sha256_init(&context);
    sha256_update(&context, outer_padding, 64);
    sha256_update(&context, inner_hash, 32);
    sha256_final(&context, final_hash);

    for (i = 0; i < 32; i++)
    {
        sprintf(hexadecimal + (i * 2), "%02x", final_hash[i]);
    }

    hexadecimal[64] = '\0';
}

static char *read_complete_file(const char *path, size_t *length)
{
    FILE *file;
    long file_size;
    char *content;
    size_t bytes_read;

    file = fopen(path, "rb");

    if (file == NULL)
    {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }

    file_size = ftell(file);

    if (file_size < 0)
    {
        fclose(file);
        return NULL;
    }

    rewind(file);

    content = malloc((size_t)file_size + 1);

    if (content == NULL)
    {
        fclose(file);
        return NULL;
    }

    bytes_read = fread(content, 1, (size_t)file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size)
    {
        free(content);
        return NULL;
    }

    content[bytes_read] = '\0';
    *length = bytes_read;

    return content;
}

static int verify_document(const char *path)
{
    const char *label = "- **HMAC-SHA-256 de autenticidad:** `";
    const char *placeholder =
        "0000000000000000000000000000000000000000000000000000000000000000";
    char stored_hash[65];
    char calculated_hash[65];
    char *content;
    char *position;
    size_t length;

    content = read_complete_file(path, &length);

    if (content == NULL)
    {
        fprintf(stderr, "[ERROR] No se pudo leer el documento: %s\n", path);
        return 1;
    }

    position = strstr(content, label);

    if (position == NULL)
    {
        fprintf(stderr, "[ERROR] El documento no contiene un HMAC reconocible.\n");
        free(content);
        return 1;
    }

    position += strlen(label);

    if (strlen(position) < 64)
    {
        fprintf(stderr, "[ERROR] El campo HMAC está incompleto.\n");
        free(content);
        return 1;
    }

    memcpy(stored_hash, position, 64);
    stored_hash[64] = '\0';

    memcpy(position, placeholder, 64);

    hmac_sha256(
        (const unsigned char *)SECRETO_JOCARSA,
        strlen(SECRETO_JOCARSA),
        (const unsigned char *)content,
        length,
        calculated_hash
    );

    free(content);

    printf("HMAC almacenado : %s\n", stored_hash);
    printf("HMAC calculado  : %s\n", calculated_hash);

    if (strcmp(stored_hash, calculated_hash) == 0)
    {
        printf("[OK] Documento auténtico e íntegro.\n");
        return 0;
    }

    printf("[ERROR] El HMAC no coincide. El documento ha sido modificado o no fue generado con el mismo secreto.\n");
    return 2;
}

static void generate_metadata_header(Buffer *out, const char *root, const char *hash_placeholder)
{
    char hostname[256];
    char current_directory[PATH_MAX];
    char date_text[64];
    const char *username;
#ifndef _WIN32
    struct passwd *user_info;
    struct utsname system_info;
#endif
    time_t now;
    struct tm local_time;

    username = "desconocido";

#ifdef _WIN32
    wchar_t user[256], computer[256];
    DWORD user_size = 256, computer_size = 256;
    char *windows_user = GetUserNameW(user, &user_size) ? win_utf8(user) : NULL;
    if (windows_user) username = windows_user;
    char *windows_host = GetComputerNameW(computer, &computer_size) ? win_utf8(computer) : NULL;
    snprintf(hostname, sizeof(hostname), "%s", windows_host ? windows_host : "desconocido");
    free(windows_host);
#else
    user_info = getpwuid(getuid());

    if (user_info != NULL && user_info->pw_name != NULL)
    {
        username = user_info->pw_name;
    }
    else
    {
        const char *environment_user;

        environment_user = getenv("USER");

        if (environment_user != NULL && environment_user[0] != '\0')
        {
            username = environment_user;
        }
    }

    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        strcpy(hostname, "desconocido");
    }

#endif
    hostname[sizeof(hostname) - 1] = '\0';

    if (getcwd(current_directory, sizeof(current_directory)) == NULL)
    {
        strcpy(current_directory, "desconocido");
    }

    now = time(NULL);
    localtime_r(&now, &local_time);
    strftime(date_text, sizeof(date_text), "%Y-%m-%d %H:%M:%S %z", &local_time);

    buf_append(out, "# Reporte de proyecto\n\n");
    buf_append(out, "## Información de generación\n\n");
    buf_printf(out, "- **Fecha:** %s\n", date_text);
    buf_printf(out, "- **Usuario:** %s\n", username);
#ifndef _WIN32
    buf_printf(out, "- **UID:** %ld\n", (long)getuid());
#endif
    buf_printf(out, "- **Equipo:** %s\n", hostname);

#ifdef _WIN32
    buf_append(out, "- **Sistema operativo:** Windows\n");
    buf_append(out, "- **Arquitectura:** x86_64\n");
    free(windows_user);
#else
    if (uname(&system_info) == 0)
    {
        buf_printf(out, "- **Sistema operativo:** %s\n", system_info.sysname);
        buf_printf(out, "- **Versión del kernel:** %s\n", system_info.release);
        buf_printf(out, "- **Arquitectura:** %s\n", system_info.machine);
    }

#endif
    buf_printf(out, "- **Directorio de ejecución:** `%s`\n", current_directory);
    buf_printf(out, "- **Proyecto documentado:** `%s`\n", root);
    buf_printf(out, "- **HMAC-SHA-256 de autenticidad:** `%s`\n", hash_placeholder);
    buf_append(out, "\n");
    buf_append(out, "> El HMAC-SHA-256 se calcula sobre el documento completo usando un secreto incluido en el programa y 64 ceros en el propio campo del HMAC. ");
    buf_append(out, "El secreto no se escribe en el informe. Este mecanismo permite comprobar integridad y que el documento fue generado con el mismo secreto.\n\n");
}

static const char *EXCLUDED_DIRS[] =  {
    ".git", "node_modules", "vendor", "venv", "__pycache__",     "modelo_entrenado", ".venv", "dist", "documentacion", NULL
}
;
static const char *ALLOWED_EXTS[] =  {
    ".html", ".css", ".js", ".php", ".py", ".java", ".sql",     ".c", ".cpp", ".cu", ".h", ".json", ".xml", ".md", ".noema",     ".cuh", ".hpp", ".mjs", NULL
}
;
static const char *SQLITE_EXTS[] =  {
    ".db", ".sqlite", ".sqlite3", NULL
}
;
typedef struct  {
    const char *ext, *lang;
}
LangMap;
static const LangMap LANG_MAP[] =  {
    {
        ".html","html"
    }
    , {
        ".css","css"
    }
    , {
        ".js","js"
    }
    , {
        ".php","php"
    }
    ,      {
        ".py","python"
    }
    , {
        ".java","java"
    }
    , {
        ".sql","sql"
    }
    , {
        ".c","c"
    }
    ,      {
        ".cpp","cpp"
    }
    , {
        ".cu","cuda"
    }
    , {
        ".h","c"
    }
    , {
        ".json","json"
    }
    ,      {
        ".xml","xml"
    }
    , {
        ".md","markdown"
    }
    , {
        NULL,NULL
    }
}
;
static void die_oom(void)  {
    fprintf(stderr, "[ERROR] Sin memoria.\n");
    exit(2);
}
static char *xstrdup(const char *s)  {
    char *p = strdup(s ? s : "");
    if (!p) die_oom();
    return p;
}
static void buf_init(Buffer *b)  {
    b->data=NULL;
    b->len=0;
    b->cap=0;
}
static void buf_reserve(Buffer *b, size_t need)  {
    if (need <= b->cap) return;
    size_t nc = b->cap ? b->cap : 4096;
    while (nc < need) nc *= 2;
    char *p = realloc(b->data, nc);
    if (!p) die_oom();
    b->data=p;
    b->cap=nc;
}
static void buf_append_n(Buffer *b, const char *s, size_t n)  {
    buf_reserve(b, b->len+n+1);
    memcpy(b->data+b->len,s,n);
    b->len+=n;
    b->data[b->len]='\0';
}
static void buf_append(Buffer *b, const char *s)  {
    buf_append_n(b,s,strlen(s));
}
static void buf_printf(Buffer *b, const char *fmt, ...)  {
    va_list ap, cp;
    va_start(ap,fmt);
    va_copy(cp,ap);
    int n=vsnprintf(NULL,0,fmt,cp);
    va_end(cp);
    if(n<0) {
        va_end(ap);
        return;
    }
    buf_reserve(b,b->len+(size_t)n+1);
    vsnprintf(b->data+b->len,(size_t)n+1,fmt,ap);
    b->len+=(size_t)n;
    va_end(ap);
}
static void buf_free(Buffer *b)  {
    free(b->data);
    b->data=NULL;
    b->len=b->cap=0;
}
static void vec_push(StrVec *v, const char *s)  {
    if(v->n==v->cap) {
        size_t nc=v->cap?v->cap*2:32;
        char **p=realloc(v->v,nc*sizeof(*p));
        if(!p)die_oom();
        v->v=p;
        v->cap=nc;
    }
    v->v[v->n++]=xstrdup(s);
}
static void vec_free(StrVec *v) {
    for(size_t i=0;i<v->n;i++)free(v->v[i]);
    free(v->v);
    v->v=NULL;
    v->n=v->cap=0;
}
static int cmp_strp(const void *a,const void *b) {
    return strcmp(*(char* const*)a,*(char* const*)b);
}
static void vec_sort(StrVec *v) {
    qsort(v->v,v->n,sizeof(char*),cmp_strp);
}
static bool ends_with_ci(const char *s,const char *suffix) {
    size_t a=strlen(s),b=strlen(suffix);
    if(b>a)return false;
    s+=a-b;
    for(size_t i=0;i<b;i++) {
        char x=s[i],y=suffix[i];
        if(x>='A'&&x<='Z')x+=32;
        if(y>='A'&&y<='Z')y+=32;
        if(x!=y)return false;
    }
    return true;
}
static bool matches_exts(const char *name,const char **exts) {
    for(size_t i=0;exts[i];i++)if(ends_with_ci(name,exts[i]))return true;
    return false;
}
static const char *file_ext(const char *name) {
    const char *p=strrchr(name,'.');
    return p?p:"";
}
static const char *lang_for(const char *name) {
    const char *e=file_ext(name);
    for(size_t i=0;LANG_MAP[i].ext;i++)if(strcasecmp(e,LANG_MAP[i].ext)==0)return LANG_MAP[i].lang;
    return "";
}
static bool excluded_dir_name(const char *name) {
    for(size_t i=0;EXCLUDED_DIRS[i];i++)if(strcmp(name,EXCLUDED_DIRS[i])==0)return true;
    return false;
}
static void path_join(char out[PATH_MAX],const char *a,const char *b) {
    snprintf(out,PATH_MAX,"%s%s%s",a,(a[0]&&a[strlen(a)-1]=='/')?"":"/",b);
}
static const char *base_name(const char *p) {
    const char *s=strrchr(p,'/');
    return s?s+1:p;
}
static bool is_dir(const char *p) {
#ifdef _WIN32
    DWORD a = win_attributes(p);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) && !(a & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    struct stat st;
    return stat(p,&st)==0&&S_ISDIR(st.st_mode);
#endif
}
static bool is_file(const char *p) {
#ifdef _WIN32
    DWORD a = win_attributes(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
#else
    struct stat st;
    return stat(p,&st)==0&&S_ISREG(st.st_mode);
#endif
}
static bool marker_excluded(const char *dir) {
    char p[PATH_MAX];
    path_join(p,dir,EXCLUSION_MARKER);
    return is_file(p);
}
static bool relative_path(const char *base,const char *path,char out[PATH_MAX]) {
    size_t n=strlen(base);
    if(strncmp(base,path,n)==0&&(path[n]=='/'||path[n]=='\0')) {
        const char *p=path+n;
        if(*p=='/')p++;
        snprintf(out,PATH_MAX,"%s",*p?p:".");
        return true;
    }
    snprintf(out,PATH_MAX,"%s",path);
    return false;
}
static void list_entries(const char *dir,StrVec *out) {
#ifdef _WIN32
    char pattern[PATH_MAX];
    path_join(pattern, dir, "*");
    wchar_t *wide = win_wide(pattern);
    WIN32_FIND_DATAW entry;
    HANDLE handle = wide ? FindFirstFileW(wide, &entry) : INVALID_HANDLE_VALUE;
    free(wide);
    if (handle == INVALID_HANDLE_VALUE) return;
    do {
        if (!wcscmp(entry.cFileName, L".") || !wcscmp(entry.cFileName, L"..")) continue;
        char *name = win_utf8(entry.cFileName);
        if (name) { vec_push(out, name); free(name); }
    } while (FindNextFileW(handle, &entry));
    FindClose(handle);
#else
    DIR *d=opendir(dir);
    if(!d)return;
    struct dirent *e;
    while((e=readdir(d))) {
        if(strcmp(e->d_name,".")==0||strcmp(e->d_name,"..")==0)continue;
        vec_push(out,e->d_name);
    }
    closedir(d);
#endif
    vec_sort(out);
}
static char *read_file(const char *path,size_t *out_len) {
    FILE *f=fopen(path,"rb");
    if(!f)return NULL;
    if(fseek(f,0,SEEK_END)!=0) {
        fclose(f);
        return NULL;
    }
    long n=ftell(f);
    if(n<0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    char *p=malloc((size_t)n+1);
    if(!p)die_oom();
    size_t got=fread(p,1,(size_t)n,f);
    p[got]='\0';
    fclose(f);
    if(out_len)*out_len=got;
    return p;
}
static bool looks_sqlite(const char *path) {
    unsigned char h[16];
    FILE *f=fopen(path,"rb");
    if(!f)return false;
    size_t n=fread(h,1,16,f);
    fclose(f);
    return n==16&&memcmp(h,"SQLite format 3\0",16)==0;
}
static char *quote_ident(const char *s) {
    Buffer b;
    buf_init(&b);
    buf_append(&b,"\"");
    for(;*s;s++) {
        if(*s=='\"')buf_append(&b,"\"\"");
        else buf_append_n(&b,s,1);
    }
    buf_append(&b,"\"");
    return b.data;
}
static void build_tree_rec(const char *dir,const char *prefix,Buffer *out) {
    StrVec all= {
        0
    }
    ,visible= {
        0
    }
    ;
    list_entries(dir,&all);
    for(size_t i=0;i<all.n;i++) {
        char p[PATH_MAX];
        path_join(p,dir,all.v[i]);
        if(is_dir(p)&&(excluded_dir_name(all.v[i])||marker_excluded(p)))continue;
        if(is_file(p)&&strcmp(all.v[i],EXCLUSION_MARKER)==0)continue;
        vec_push(&visible,all.v[i]);
    }
    for(size_t i=0;i<visible.n;i++) {
        bool last=i+1==visible.n;
        char p[PATH_MAX];
        path_join(p,dir,visible.v[i]);
        buf_printf(out,"%s%s%s%s\n",prefix,last?"└── ":"├── ",visible.v[i],is_file(p)&&matches_exts(visible.v[i],SQLITE_EXTS)?" [SQLite]":"");
        if(is_dir(p)) {
            Buffer np;
            buf_init(&np);
            buf_printf(&np,"%s%s",prefix,last?"    ":"│   ");
            build_tree_rec(p,np.data,out);
            buf_free(&np);
        }
    }
    vec_free(&visible);
    vec_free(&all);
}
static void build_tree(const char *root,Buffer *out) {
    buf_printf(out,"%s\n",root);
    build_tree_rec(root,"",out);
}
static void find_sqlite_rec(const char *dir,StrVec *dbs) {
    if(marker_excluded(dir))return;
    StrVec e= {
        0
    }
    ;
    list_entries(dir,&e);
    for(size_t i=0;i<e.n;i++) {
        char p[PATH_MAX];
        path_join(p,dir,e.v[i]);
        if(is_dir(p)) {
            if(!excluded_dir_name(e.v[i])&&!marker_excluded(p))find_sqlite_rec(p,dbs);
        }
        else if(is_file(p)&&matches_exts(e.v[i],SQLITE_EXTS))vec_push(dbs,p);
    }
    vec_free(&e);
}
static void sqlite_schema(const char *db,const char *rel,Buffer *out) {
    buf_printf(out,"%s\n",rel);
    if(!looks_sqlite(db)) {
        buf_append(out,"└── [omitida] El archivo no parece ser una base SQLite válida\n");
        return;
    }
    sqlite3 *conn=NULL;
    if(sqlite3_open_v2(db,&conn,SQLITE_OPEN_READONLY,NULL)!=SQLITE_OK) {
        buf_printf(out,"└── [ERROR] No se pudo inspeccionar la base SQLite: %s\n",conn?sqlite3_errmsg(conn):"error de apertura");
        if(conn)sqlite3_close(conn);
        return;
    }
    sqlite3_stmt *st=NULL;
    const char *sql="SELECT name,type,sql FROM sqlite_master WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%' ORDER BY type,name";
    if(sqlite3_prepare_v2(conn,sql,-1,&st,NULL)!=SQLITE_OK) {
        buf_printf(out,"└── [ERROR] %s\n",sqlite3_errmsg(conn));
        sqlite3_close(conn);
        return;
    }
    typedef struct {
        char *name,*type,*sql;
    }
    Obj;
    Obj *objs=NULL;
    size_t n=0,cap=0;
    while(sqlite3_step(st)==SQLITE_ROW) {
        if(n==cap) {
            cap=cap?cap*2:16;
            objs=realloc(objs,cap*sizeof(*objs));
            if(!objs)die_oom();
        }
        objs[n].name=xstrdup((const char*)sqlite3_column_text(st,0));
        objs[n].type=xstrdup((const char*)sqlite3_column_text(st,1));
        const unsigned char *q=sqlite3_column_text(st,2);
        objs[n].sql=xstrdup(q?(const char*)q:"");
        n++;
    }
    sqlite3_finalize(st);
    if(!n)buf_append(out,"└── [sin tablas ni vistas de usuario]\n");
    for(size_t i=0;i<n;i++) {
        bool last=i+1==n;
        const char *co=last?"└── ":"├── ";
        const char *po=last?"    ":"│   ";
        if(strcmp(objs[i].type,"table")==0) {
            buf_printf(out,"%stabla %s\n",co,objs[i].name);
            char *qi=quote_ident(objs[i].name);
            Buffer q;
            buf_init(&q);
            buf_printf(&q,"PRAGMA table_info(%s)",qi);
            free(qi);
            StrVec cols= {
                0
            }
            ,fks= {
                0
            }
            ,idxs= {
                0
            }
            ;
            if(sqlite3_prepare_v2(conn,q.data,-1,&st,NULL)==SQLITE_OK) {
                while(sqlite3_step(st)==SQLITE_ROW) {
                    Buffer x;
                    buf_init(&x);
                    const char *nm=(const char*)sqlite3_column_text(st,1);
                    const char *ty=(const char*)sqlite3_column_text(st,2);
                    int nn=sqlite3_column_int(st,3),pk=sqlite3_column_int(st,5);
                    const char *def=(const char*)sqlite3_column_text(st,4);
                    buf_printf(&x,"%s",nm?nm:"");
                    if(ty&&*ty)buf_printf(&x," %s",ty);
                    if(pk)buf_printf(&x,pk==1?" PRIMARY KEY":" PRIMARY KEY(%d)",pk);
                    if(nn)buf_append(&x," NOT NULL");
                    if(def)buf_printf(&x," DEFAULT %s",def);
                    vec_push(&cols,x.data);
                    buf_free(&x);
                }
                sqlite3_finalize(st);
            }
            buf_free(&q);
            qi=quote_ident(objs[i].name);
            buf_init(&q);
            buf_printf(&q,"PRAGMA foreign_key_list(%s)",qi);
            free(qi);
            if(sqlite3_prepare_v2(conn,q.data,-1,&st,NULL)==SQLITE_OK) {
                while(sqlite3_step(st)==SQLITE_ROW) {
                    Buffer x;
                    buf_init(&x);
                    buf_printf(&x,"%s → %s.%s (ON UPDATE %s, ON DELETE %s)",sqlite3_column_text(st,3),sqlite3_column_text(st,2),sqlite3_column_text(st,4),sqlite3_column_text(st,5),sqlite3_column_text(st,6));
                    vec_push(&fks,x.data);
                    buf_free(&x);
                }
                sqlite3_finalize(st);
            }
            buf_free(&q);
            qi=quote_ident(objs[i].name);
            buf_init(&q);
            buf_printf(&q,"PRAGMA index_list(%s)",qi);
            free(qi);
            if(sqlite3_prepare_v2(conn,q.data,-1,&st,NULL)==SQLITE_OK) {
                while(sqlite3_step(st)==SQLITE_ROW) {
                    const char *in=(const char*)sqlite3_column_text(st,1);
                    int uniq=sqlite3_column_int(st,2);
                    int partial=sqlite3_column_count(st)>4?sqlite3_column_int(st,4):0;
                    char *ii=quote_ident(in);
                    Buffer qq;
                    buf_init(&qq);
                    buf_printf(&qq,"PRAGMA index_info(%s)",ii);
                    free(ii);
                    sqlite3_stmt *si=NULL;
                    Buffer cs;
                    buf_init(&cs);
                    if(sqlite3_prepare_v2(conn,qq.data,-1,&si,NULL)==SQLITE_OK) {
                        bool first=true;
                        while(sqlite3_step(si)==SQLITE_ROW) {
                            const char *cn=(const char*)sqlite3_column_text(si,2);
                            if(!first)buf_append(&cs,", ");
                            buf_append(&cs,cn?cn:"");
                            first=false;
                        }
                        sqlite3_finalize(si);
                    }
                    Buffer x;
                    buf_init(&x);
                    buf_printf(&x,"%s",in?in:"");
                    if(uniq||partial) {
                        buf_append(&x," (");
                        if(uniq)buf_append(&x,"UNIQUE");
                        if(uniq&&partial)buf_append(&x,", ");
                        if(partial)buf_append(&x,"PARTIAL");
                        buf_append(&x,")");
                    }
                    buf_printf(&x,": %s",cs.len?cs.data:"sin columnas detectadas");
                    vec_push(&idxs,x.data);
                    buf_free(&x);
                    buf_free(&cs);
                    buf_free(&qq);
                }
                sqlite3_finalize(st);
            }
            buf_free(&q);
            int blocks=1+(fks.n?1:0)+(idxs.n?1:0),bi=0;
            StrVec *sets[3]= {
                &cols,&fks,&idxs
            }
            ;
            const char *titles[3]= {
                "columnas","claves foráneas","índices"
            }
            ;
            for(int k=0;k<3;k++) {
                if(k>0&&sets[k]->n==0)continue;
                bool lb=(++bi==blocks);
                buf_printf(out,"%s%s%s\n",po,lb?"└── ":"├── ",titles[k]);
                const char *pb=lb?"    ":"│   ";
                if(!sets[k]->n)buf_printf(out,"%s%s└── [sin elementos]\n",po,pb);
                for(size_t j=0;j<sets[k]->n;j++)buf_printf(out,"%s%s%s%s\n",po,pb,j+1==sets[k]->n?"└── ":"├── ",sets[k]->v[j]);
            }
            vec_free(&cols);
            vec_free(&fks);
            vec_free(&idxs);
        }
        else  {
            buf_printf(out,"%svista %s\n",co,objs[i].name);
            if(objs[i].sql[0]) {
                Buffer clean;
                buf_init(&clean);
                bool ws=false;
                for(const char *p=objs[i].sql;*p;p++) {
                    if(*p==' '||*p=='\n'||*p=='\r'||*p=='\t') {
                        ws=true;
                    }
                    else {
                        if(ws&&clean.len)buf_append(&clean," ");
                        buf_append_n(&clean,p,1);
                        ws=false;
                    }
                }
                if(clean.len>500) {
                    clean.len=500;
                    clean.data[500]='\0';
                    buf_append(&clean,"…");
                }
                buf_printf(out,"%s└── SQL: %s\n",po,clean.data?clean.data:"");
                buf_free(&clean);
            }
        }
        free(objs[i].name);
        free(objs[i].type);
        free(objs[i].sql);
    }
    free(objs);
    sqlite3_close(conn);
}
static void generate_sqlite_doc(const char *root,Buffer *out) {
    buf_append(out,"## Bases de datos SQLite\n\nEsta sección documenta únicamente el esquema de las bases SQLite detectadas. No se vuelcan registros ni datos de usuario.\n\n");
    StrVec dbs= {
        0
    }
    ;
    find_sqlite_rec(root,&dbs);
    vec_sort(&dbs);
    if(!dbs.n) {
        buf_append(out,"No se han encontrado bases SQLite con extensiones .db, .sqlite o .sqlite3.\n");
        vec_free(&dbs);
        return;
    }
    for(size_t i=0;i<dbs.n;i++) {
        char rel[PATH_MAX];
        relative_path(root,dbs.v[i],rel);
        buf_printf(out,"### %s\n\n```text\n",rel);
        sqlite_schema(dbs.v[i],rel,out);
        buf_append(out,"```\n\n");
    }
    vec_free(&dbs);
}
static void report_interleaved(const char *dir,int level,const char *base,Buffer *out) {
    if(marker_excluded(dir))return;
    for(int i=0;i<level;i++)buf_append(out,"#");
    buf_printf(out," %s\n",base_name(dir));
    StrVec e= {
        0
    }
    ;
    list_entries(dir,&e);
    for(size_t i=0;i<e.n;i++) {
        char p[PATH_MAX];
        path_join(p,dir,e.v[i]);
        if(is_file(p)&&matches_exts(e.v[i],ALLOWED_EXTS)) {
            size_t n=0;
            char *content=read_file(p,&n);
            buf_printf(out,"**%s**\n",e.v[i]);
            if(!content) {
                buf_printf(out,"```%s\nError al leer el archivo: %s\n```\n",lang_for(e.v[i]),strerror(errno));
            }
            else {
                buf_printf(out,"```%s\n",lang_for(e.v[i]));
                buf_append_n(out,content,n);
                if(n&&content[n-1]!='\n')buf_append(out,"\n");
                buf_append(out,"```\n");
                free(content);
            }
        }
    }
    for(size_t i=0;i<e.n;i++) {
        char p[PATH_MAX];
        path_join(p,dir,e.v[i]);
        if(is_dir(p)&&!excluded_dir_name(e.v[i])&&!marker_excluded(p))report_interleaved(p,level+1,base,out);
    }
    vec_free(&e);
}
static void generate_report(const char *root,Buffer *out) {
    Buffer tree;
    buf_init(&tree);
    build_tree(root,&tree);
    buf_append(out,"## Estructura del proyecto\n\n```\n");
    buf_append(out,tree.data?tree.data:"");
    buf_append(out,"```\n\n");
    buf_free(&tree);
    generate_sqlite_doc(root,out);
    buf_append(out,"\n## Código (intercalado)\n\n");
    report_interleaved(root,1,root,out);
}
static int mkdir_p(const char *path) {
    char tmp[PATH_MAX];
    snprintf(tmp,sizeof(tmp),"%s",path);
    size_t n=strlen(tmp);
    if(n&&tmp[n-1]=='/')tmp[n-1]='\0';
    char *start = tmp + 1;
#ifdef _WIN32
    if (tmp[0] && tmp[1] == ':') start = tmp + 3;
    else if (tmp[0] == '/' && tmp[1] == '/') {
        start = strchr(tmp + 2, '/');
        if (start) start = strchr(start + 1, '/');
        if (!start) return is_dir(tmp) ? 0 : -1;
        start++;
    }
#endif
    for(char *p=start;*p;p++)if(*p=='/') {
        *p='\0';
        if(mkdir(tmp,0755)!=0&&errno!=EEXIST)return -1;
        *p='/';
    }
    if(mkdir(tmp,0755)!=0&&errno!=EEXIST)return -1;
    return 0;
}
static int run(int argc, char **argv)
{
    char root[PATH_MAX];
    char dest[PATH_MAX];
    char timestamp[32];
    char hash[65];
    char *output_path;
    const char *project_name;
    size_t output_path_length;
    time_t now;
    struct tm local_time;
    Buffer body;
    Buffer report;
    FILE *file;

    const char *hash_placeholder =
        "0000000000000000000000000000000000000000000000000000000000000000";

    if (argc == 3 && strcmp(argv[1], "--verify") == 0)
    {
        return verify_document(argv[2]);
    }

    if (argc != 3)
    {
        fprintf(stderr, "Uso para generar: %s <carpeta_origen> <carpeta_destino>\n", argv[0]);
        fprintf(stderr, "Uso para verificar: %s --verify <documento.md>\n", argv[0]);
        return 1;
    }

    if (realpath(argv[1], root) == NULL)
    {
        fprintf(
            stderr,
            "[ERROR] La carpeta origen no existe o no es accesible: %s\n",
            argv[1]
        );

        return 1;
    }

    if (!is_dir(root))
    {
        fprintf(stderr, "[ERROR] La ruta origen no es un directorio: %s\n", root);
        return 1;
    }

    if (excluded_dir_name(base_name(root)))
    {
        fprintf(
            stderr,
            "[ERROR] La carpeta origen está excluida por configuración: %s\n",
            root
        );

        return 1;
    }

    if (marker_excluded(root))
    {
        fprintf(
            stderr,
            "[ERROR] La carpeta origen contiene %s y está excluida: %s\n",
            EXCLUSION_MARKER,
            root
        );

        return 1;
    }

#ifdef _WIN32
    if (!win_fullpath(argv[2], dest)) {
        fprintf(stderr, "[ERROR] Ruta de destino no valida.\n");
        return 2;
    }
#else
    if (argv[2][0] == '/')
    {
        snprintf(dest, sizeof(dest), "%s", argv[2]);
    }
    else
    {
        char current_directory[PATH_MAX];

        if (getcwd(current_directory, sizeof(current_directory)) == NULL)
        {
            perror("getcwd");
            return 1;
        }

        path_join(dest, current_directory, argv[2]);
    }

#endif
    if (mkdir_p(dest) != 0)
    {
        fprintf(
            stderr,
            "[ERROR] No se pudo crear destino %s: %s\n",
            dest,
            strerror(errno)
        );

        return 2;
    }

    now = time(NULL);
    localtime_r(&now, &local_time);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", &local_time);

    project_name = base_name(root);

    if (project_name[0] == '\0')
    {
        project_name = "reporte";
    }

    output_path_length =
        strlen(dest) +
        1 +
        strlen(project_name) +
        1 +
        strlen(timestamp) +
        strlen(".md") +
        1;

    output_path = malloc(output_path_length);

    if (output_path == NULL)
    {
        fprintf(stderr, "[ERROR] No se pudo reservar memoria para la ruta de salida.\n");
        return 2;
    }

    snprintf(
        output_path,
        output_path_length,
        "%s/%s_%s.md",
        dest,
        project_name,
        timestamp
    );

    buf_init(&body);
    generate_report(root, &body);

    buf_init(&report);
    generate_metadata_header(&report, root, hash_placeholder);
    buf_append_n(&report, body.data, body.len);

    /*
       Calculamos el HMAC con el campo de autenticidad de la cabecera a cero.
       Después sustituimos esos 64 ceros por el hash calculado.
    */
    hmac_sha256(
        (const unsigned char *)SECRETO_JOCARSA,
        strlen(SECRETO_JOCARSA),
        (const unsigned char *)report.data,
        report.len,
        hash
    );

    {
        char *hash_position;

        hash_position = strstr(report.data, hash_placeholder);

        if (hash_position == NULL)
        {
            fprintf(stderr, "[ERROR] No se encontró el campo reservado para el hash.\n");
            buf_free(&body);
            buf_free(&report);
            free(output_path);
            return 2;
        }

        memcpy(hash_position, hash, 64);
    }

    file = fopen(output_path, "wb");

    if (file == NULL)
    {
        fprintf(
            stderr,
            "[ERROR] No se pudo guardar %s: %s\n",
            output_path,
            strerror(errno)
        );

        buf_free(&body);
        buf_free(&report);
        free(output_path);
        return 2;
    }

    if (report.len > 0)
    {
        size_t bytes_written;

        bytes_written = fwrite(report.data, 1, report.len, file);

        if (bytes_written != report.len)
        {
            fprintf(stderr, "[ERROR] Escritura incompleta.\n");
            fclose(file);
            buf_free(&body);
            buf_free(&report);
            free(output_path);
            return 2;
        }
    }

    fclose(file);

    printf("[OK] Reporte generado: %s\n", output_path);
    printf("[OK] HMAC-SHA-256: %s\n", hash);

    buf_free(&body);
    buf_free(&report);
    free(output_path);

    return 0;
}

#ifdef _WIN32
int wmain(int argc, wchar_t **wide_argv) {
    SetConsoleOutputCP(CP_UTF8);
    char **argv = calloc((size_t)argc + 1, sizeof(char *));
    if (!argv) return 2;
    for (int i = 0; i < argc; i++) {
        argv[i] = win_utf8(wide_argv[i]);
        if (!argv[i]) {
            for (int j = 0; j < i; j++) free(argv[j]);
            free(argv); return 2;
        }
    }
    int result = run(argc, argv);
    for (int i = 0; i < argc; i++) free(argv[i]);
    free(argv);
    return result;
}
#else
int main(int argc, char **argv) { return run(argc, argv); }
#endif
