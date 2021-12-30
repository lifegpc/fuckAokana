#include "archive.h"

#include <fcntl.h>
#include <list>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cstr_util.h"
#include "fileop.h"
#include "linked_list.h"

#ifndef _O_BINARY
#if _WIN32
#define _O_BINARY 0x8000
#else
#define _O_BINARY 0
#endif
#endif

#ifndef _SH_DENYWR
#define _SH_DENYWR 0x20
#endif

#ifndef _S_IREAD
#define _S_IREAD 0x100
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

typedef struct aokana_file_info {
    std::string file_name;
    uint32_t key = 0;
    uint32_t position = 0;
    uint32_t len = 0;
} aokana_file_info;

typedef struct aokana_file: public aokana_file_info {
    aokana_archive* arc;
    uint8_t* key_buf;
    uint32_t pos = 0;
} aokana_file;

typedef struct aokana_archive {
    struct LinkedList<aokana_file_info>* list;
    uint32_t file_count;
    size_t file_size;
    FILE* f;
} aokana_archive;

aokana_arc_error arc_decrypt(uint8_t* data, size_t len, uint32_t key, uint8_t*& keybuf, uint32_t pos = 0) {
    if (!data) return AOKANA_ARC_NULL_POINTER;
    if (!keybuf) {
        keybuf = (uint8_t*)malloc(256);
        if (!keybuf) {
            return AOKANA_ARC_OOM;
        }
        uint32_t a = key * 7391U + 42828U, b = a << 17 ^ a;
        for (int i = 0; i < 256; i++) {
            a -= key;
            a += b;
            b = a + 56U;
            a *= (b & 239U);
            keybuf[i] = (uint8_t)a;
            a >>= 1;
        }
    }
    uint8_t t;
    for (int i = 0; i < len; i++) {
        t = data[i];
        t ^= keybuf[(i + pos) % 253];
        t += 3;
        t += keybuf[(i + pos) % 89];
        t ^= 153;
        data[i] = t;
    }
    return AOKANA_ARC_OK;
}

aokana_arc_error open_archive(aokana_archive** archive, const char* path) {
    if (!archive || !path) return AOKANA_ARC_NULL_POINTER;
    return open_archive(*archive, path);
}

void free_archive(aokana_archive* archive) {
    if (!archive) return;
    if (archive->f) fileop::fclose(archive->f);
    linked_list_clear(archive->list);
    free(archive);
}

void free_aokana_file(aokana_file* f) {
    if (!f) return;
    if (f->key_buf) free(f->key_buf);
    free(f);
}

aokana_arc_error open_archive(aokana_archive*& archive, std::string path) {
    if (!fileop::exists(path)) {
        return AOKANA_ARC_NOT_FOUND;
    }
    aokana_archive* arc = (aokana_archive*)malloc(sizeof(aokana_archive));
    if (!arc) {
        return AOKANA_ARC_OOM;
    }
    memset(arc, 0, sizeof(aokana_archive));
    if (!fileop::get_file_size(path, arc->file_size)) {
        free_archive(arc);
        return AOKANA_ARC_UNKNOWN_SIZE;
    }
    int fd;
    if (fileop::open(path, fd, O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD)) {
        free_archive(arc);
        return AOKANA_ARC_OPEN_FAILED;
    }
    arc->f = fileop::fdopen(fd, "r");
    if (!arc->f) {
        fileop::close(fd);
        free_archive(arc);
        return AOKANA_ARC_OPEN_FAILED;
    }
    uint32_t key, key2;
    uint8_t* keybuf = nullptr, * keybuf2 = nullptr;
    {
        uint8_t buf[1024];
        if (fread(buf, 1, 1024, arc->f) < 1024) {
            free_archive(arc);
            return AOKANA_ARC_TOO_SMALL;
        }
        for (int i = 4; i < 255; i++) {
            arc->file_count += cstr_read_uint32(buf + (size_t)i * 4, 0);
        }
        key = cstr_read_uint32(buf + 212, 0);
        key2 = cstr_read_uint32(buf + 92, 0);
    }
    size_t buf2_size = (size_t)arc->file_count * 16;
    aokana_arc_error re = AOKANA_ARC_OK;
    uint8_t* buf2 = nullptr, * buf3 = nullptr;
    uint32_t meta_size, buf3_size;
    struct LinkedList<aokana_file_info>* tail = nullptr;
    size_t data_size = 0;
    if (1024 + buf2_size > arc->file_size) {
        re = AOKANA_ARC_TOO_SMALL;
        goto end;
    }
    buf2 = (uint8_t*)malloc(buf2_size);
    if (!buf2) {
        re = AOKANA_ARC_OOM;
        goto end;
    }
    if (fread(buf2, 1, buf2_size, arc->f) < buf2_size) {
        re = AOKANA_ARC_TOO_SMALL;
        goto end;
    }
    if ((re = arc_decrypt(buf2, buf2_size, key, keybuf))) {
        goto end;
    }
    meta_size = cstr_read_uint32(buf2 + 12, 0);
    buf3_size = meta_size - (1024 + buf2_size);
    buf3 = (uint8_t*)malloc(buf3_size);
    if (!buf3) {
        re = AOKANA_ARC_OOM;
        goto end;
    }
    if (fread(buf3, 1, buf3_size, arc->f) < buf3_size) {
        re = AOKANA_ARC_TOO_SMALL;
        goto end;
    }
    if ((re = arc_decrypt(buf3, buf3_size, key2, keybuf2))) {
        goto end;
    }
    for (uint32_t i = 0; i < arc->file_count; i++) {
        size_t metapos = (size_t)i * 16;
        aokana_file_info inf;
        inf.len = cstr_read_uint32(buf2 + metapos, 0);
        uint32_t s = cstr_read_uint32(buf2 + metapos + 4, 0);
        inf.file_name = std::string((char*)buf3 + s, min(strlen((char*)buf3 + s), buf3_size - s));
        inf.key = cstr_read_uint32(buf2 + metapos + 8, 0);
        inf.position = cstr_read_uint32(buf2 + metapos + 12, 0);
        data_size += inf.len;
        if (!tail) {
            if (!linked_list_append(arc->list, &inf, &tail)) {
                re = AOKANA_ARC_OOM;
                goto end;
            }
        } else {
            if (!linked_list_append(tail, &inf, &tail)) {
                re = AOKANA_ARC_OOM;
                goto end;
            }
        }
    }
    if (data_size + meta_size > arc->file_size) {
        re = AOKANA_ARC_TOO_SMALL;
        goto end;
    }
    archive = arc;
end:
    if (keybuf) free(keybuf);
    if (keybuf2) free(keybuf2);
    if (buf2) free(buf2);
    if (buf3) free(buf3);
    if (re) {
        free_archive(arc);
    }
    return re;
}

aokana_file* create_aokana_file(aokana_archive* arc, aokana_file_info inf) {
    aokana_file* f = (aokana_file*)malloc(sizeof(aokana_file));
    if (!f) return nullptr;
    memset(f, 0, sizeof(aokana_file));
    f->arc = arc;
    f->file_name = std::string(inf.file_name);
    f->key = inf.key;
    f->key_buf = nullptr;
    f->len = inf.len;
    f->position = inf.position;
    f->pos = 0;
    return f;
}

aokana_file* get_file_from_archive(aokana_archive* archive, uint32_t index) {
    if (!archive || index >= archive->file_count) return nullptr;
    auto t = archive->list;
    for (uint32_t i = 0; i < archive->file_count; i++) {
        if (i == index) return create_aokana_file(archive, t->d);
        t = t->next;
    }
    return nullptr;
}

uint32_t get_file_count_from_archive(aokana_archive* archive) {
    if (!archive) return 0;
    return archive->file_count;
}

size_t aokana_file_read(aokana_file* f, char* buf, size_t buf_len) {
    if (!f || !buf) return (size_t)-1;
    if (f->pos >= f->len) return (size_t)-2;
    if (fileop::fseek(f->arc->f, (int64_t)f->position + (int64_t)f->pos, SEEK_SET)) return (size_t)-1;
    size_t len = min(buf_len, f->len - f->pos);
    size_t c = fread(buf, 1, len, f->arc->f);
    if (!c) return c;
    uint8_t* t = (uint8_t*)buf;
    if (arc_decrypt(t, c, f->key, f->key_buf, f->pos)) return (size_t)-1;
    f->pos += c;
    return c;
}

std::string get_aokana_file_name(aokana_file* f) {
    if (!f) return "";
    return f->file_name;
}
