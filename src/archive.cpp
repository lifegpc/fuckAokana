#include "archive.h"

#include <errno.h>
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
#include "str_util.h"

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
#if _WIN32
#define _S_IREAD 0x100
#else
#define _S_IREAD 0
#endif
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

typedef struct aokana_file_info {
    char* file_name = nullptr;
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
    char* archive_name;
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

void free_aokana_file_info(aokana_file_info f) {
    if (f.file_name) free(f.file_name);
}

void free_archive(aokana_archive* archive) {
    if (!archive) return;
    if (archive->f) fileop::fclose(archive->f);
    if (archive->archive_name) free(archive->archive_name);
    linked_list_clear(archive->list, &free_aokana_file_info);
    free(archive);
}

void free_aokana_file(aokana_file* f) {
    if (!f) return;
    if (f->key_buf) free(f->key_buf);
    free_aokana_file_info(*f);
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
    if (cstr_util_copy_str(&arc->archive_name, path.c_str())) {
        free_archive(arc);
        return AOKANA_ARC_OOM;
    }
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
        size_t le = min(strlen((char*)buf3 + s), buf3_size - s);
        inf.file_name = (char*)malloc(le + 1);
        if (!inf.file_name) {
            re = AOKANA_ARC_OOM;
            goto end;
        }
        memcpy(inf.file_name, buf3 + s, le);
        inf.file_name[le] = 0;
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
    if (cstr_util_copy_str(&f->file_name, inf.file_name)) {
        free(f);
        return nullptr;
    }
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
    int64_t loc = (int64_t)f->position + (int64_t)f->pos;
    if (fileop::ftell(f->arc->f) != loc) {
        if (fileop::fseek(f->arc->f, loc, SEEK_SET)) return (size_t)-1;
    }
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

aokana_file* get_file_from_archive(aokana_archive* archive, std::string file_path) {
    if (!archive) return nullptr;
    auto t = archive->list;
    for (uint32_t i = 0; i < archive->file_count; i++) {
        if (file_path == t->d.file_name) return create_aokana_file(archive, t->d);
        t = t->next;
    }
    return nullptr;
}

std::string get_archive_name(aokana_archive* archive) {
    if (!archive) return "";
    return archive->archive_name;
}

uint32_t get_aokana_file_size(aokana_file* f) {
    if (!f) return (uint32_t)-1;
    return f->len;
}

std::string get_aokana_file_full_path(aokana_file* f) {
    if (!f) return "";
    return fileop::join(f->arc->archive_name, f->file_name);
}

bool compare_aokana_file_name(aokana_file* f1, aokana_file* f2) {
    if (!f1 || !f2) return false;
    return !strcmp(f1->file_name, f2->file_name);
}

std::string get_aokana_file_archive_name(aokana_file* f) {
    if (!f || !f->arc) return "";
    return f->arc->archive_name;
}

bool match_file_info(aokana_file_info f, std::string language, std::string file_name, bool& is_fallback) {
    std::string fn(f.file_name);
    auto fnl = str_util::str_split(fn, "/");
    if (fnl.size() < 2 || fnl.size() > 3) return false;
    std::string lang, filename;
    fnl.pop_front();
    if (fnl.size() == 1) filename = fnl.front();
    else {
        filename = fnl.back();
        lang = fnl.front();
    }
    filename = fileop::filename(filename);
    if (filename == file_name) {
        if (lang.empty()) {
            is_fallback = language.empty() ? false : true;
            return true;
        } else if (lang == language) {
            is_fallback = false;
            return true;
        }
        return false;
    }
    return false;
}

bool compare_aokana_file_info_name(aokana_file_info f1, aokana_file_info f2) {
    std::string fn1(f1.file_name), fn2(f2.file_name);
    auto fnl1 = str_util::str_split(fn1, "/"), fnl2 = str_util::str_split(fn2, "/");
    if (fnl1.front() == fnl2.front() && fnl1.back() == fnl2.back()) return true;
    return false;
}

struct LinkedList<aokana_file*>* find_files_from_archive(aokana_archive* archive, std::string language, std::string file_name, bool allow_fallback) {
    struct LinkedList<aokana_file_info>* li = nullptr, * li2 = nullptr;
    struct LinkedList<aokana_file*>* re = nullptr;
    aokana_file* f = nullptr;
    auto t = archive->list;
    bool b = false;
    if (match_file_info(t->d, language, file_name, b)) {
        if (!linked_list_append(b ? li2 : li, &t->d)) return nullptr;
    }
    while (t->next) {
        t = t->next;
        if (match_file_info(t->d, language, file_name, b)) {
            if (!linked_list_append(b ? li2 : li, &t->d)) {
                linked_list_clear(li);
                linked_list_clear(li2);
                return nullptr;
            }
        }
    }
    if (li2) {
        auto t2 = li2;
        if (!linked_list_have(li, t2->d, &compare_aokana_file_info_name)) {
            if (!linked_list_append(li, &t2->d)) {
                linked_list_clear(li);
                linked_list_clear(li2);
                return nullptr;
            }
        }
        while (t2->next) {
            t2 = t2->next;
            if (!linked_list_have(li, t2->d, &compare_aokana_file_info_name)) {
                if (!linked_list_append(li, &t2->d)) {
                    linked_list_clear(li);
                    linked_list_clear(li2);
                    return nullptr;
                }
            }
        }
    }
    if (!li) {
        linked_list_clear(li);
        linked_list_clear(li2);
        return nullptr;
    }
    t = li;
    f = create_aokana_file(archive, t->d);
    if (!f) {
        linked_list_clear(li);
        linked_list_clear(li2);
        return nullptr;
    }
    if (!linked_list_append(re, &f)) {
        free_aokana_file(f);
        linked_list_clear(li);
        linked_list_clear(li2);
        return nullptr;
    }
    while (t->next) {
        t = t->next;
        f = create_aokana_file(archive, t->d);
        if (!f) {
            linked_list_clear(re, &free_aokana_file);
            linked_list_clear(li);
            linked_list_clear(li2);
            return nullptr;
        }
        if (!linked_list_append(re, &f)) {
            free_aokana_file(f);
            linked_list_clear(re, &free_aokana_file);
            linked_list_clear(li);
            linked_list_clear(li2);
            return nullptr;
        }
    }
    linked_list_clear(li);
    linked_list_clear(li2);
    return re;
}

bool aokana_file_is_hi(aokana_file* f) {
    if (!f) return false;
    std::string n(f->file_name);
    n = str_util::str_split(n, "/").front();
    return n.length() >= 3 && n.find("_hi") == (n.length() - 3) ? true : false;
}

std::string get_aokana_file_lang(aokana_file* f) {
    if (!f) return "";
    auto s = str_util::str_split(f->file_name, "/");
    if (s.size() >= 3) {
        s.pop_back();
        return s.back();
    }
    return "";
}

bool aokana_file_is_censor(aokana_file* f) {
    if (!f) return false;
    std::string n(f->file_name);
    n = str_util::str_split(n, "/").front();
    return n.length() >= 6 && n.find("censor") != -1 ? true : false;
}

int aokana_file_compare_language(aokana_file* f, aokana_file* f2) {
    if (!f || !f2) return 0;
    std::string n(f->file_name), n2(f2->file_name);
    auto li = str_util::str_split(n, "/"), li2 = str_util::str_split(n2, "/");
    if (li.front() != li2.front()) return 0;
    li.pop_front();
    li2.pop_front();
    if (li.back() != li2.back()) return 0;
    return li.size() > li2.size() ? -1 : 1;
}

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define FFERRTAG(a, b, c, d) (-(int)MKTAG(a, b, c, d))
#define AVERROR_EOF FFERRTAG( 'E','O','F',' ')
#define AVERROR(e) (-(e))

int aokana_file_readpacket(void* f, uint8_t* buf, int buf_size) {
    size_t ret = aokana_file_read((aokana_file*)f, (char*)buf, buf_size);
    if (ret == -1) return AVERROR(EINVAL);
    else if (ret == -2) return AVERROR_EOF;
    else return ret;
}

char* c_get_aokana_file_full_path(aokana_file* f) {
    auto t = get_aokana_file_full_path(f);
    char* tmp = nullptr;
    if (cstr_util_copy_str(&tmp, t.c_str())) {
        return nullptr;
    }
    return tmp;
}
