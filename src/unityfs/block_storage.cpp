#include "unityfs_private.h"
#include "block_storage.h"
#include "fuckaokana_config.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "decompress.h"
#include "fileop.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

int64_t unityfs_archive_block_storage_tell(unityfs_archive* arc) {
    if (!arc) return -1;
    return arc->curpos;
}

int unityfs_archive_block_storage_in_current_block(unityfs_archive* arc, int64_t pos) {
    if (!arc || !arc->current_block) return 0;
    int64_t end = arc->current_block_start + arc->current_block->d.uncompress_size;
    return arc->current_block_start <= pos && pos < end ? 1 : 0;
}

int unityfs_archive_block_storage_seek_to_block(unityfs_archive* arc, int64_t pos) {
    if (!arc) return 1;
    int64_t ofs = 0, basepos = 0;
    auto t = arc->blocks;
    bool ok = false;
    if (pos >= arc->maxpos || pos < 0) {
        if (arc->curbuf) {
            free(arc->curbuf);
            arc->curbuf = nullptr;
        }
        arc->current_block = nullptr;
        arc->current_block_basepos = 0;
        arc->current_block_start = 0;
        return 0;
    }
    if (pos < ofs + t->d.uncompress_size) {
        ok = true;
    } else {
        ofs += t->d.uncompress_size;
        basepos += t->d.compress_size;
    }
    while (t->next && !ok) {
        t = t->next;
        if (pos < ofs + t->d.uncompress_size) {
            ok = true;
            break;
        } else {
            ofs += t->d.uncompress_size;
            basepos += t->d.compress_size;
        }
    }
    if (!ok) return 1;
    if (arc->current_block == t) {
        return 0;
    }
    if (arc->curbuf) {
        free(arc->curbuf);
        arc->curbuf = nullptr;
    }
    if (t->d.compression) {
        char* srcbuf = (char*)malloc(t->d.compress_size);
        if (!srcbuf) {
            printf("Out of memory.\n");
            return 1;
        }
        if (fileop::fseek(arc->f, arc->basepos + basepos, SEEK_SET)) {
            free(srcbuf);
            printf("Failed to seek in file.\n");
            return 1;
        }
        if (fread(srcbuf, 1, t->d.compress_size, arc->f) != t->d.compress_size) {
            free(srcbuf);
            printf("Failed to read from file.\n");
            return 1;
        }
        if (decompress(srcbuf, &arc->curbuf, t->d.compression, t->d.compress_size, (uint32_t*)&t->d.uncompress_size)) {
            free(srcbuf);
            printf("Failed to uncompress block data.\n");
            return 1;
        }
        free(srcbuf);
    }
    arc->current_block = t;
    arc->current_block_basepos = basepos;
    arc->current_block_start = ofs;
    return 0;
}

int unityfs_archive_block_storage_seek3(unityfs_archive* arc, int64_t pos) {
    if (!arc) return 1;
    if (pos < 0 || pos > arc->maxpos) return 1;
    if (!unityfs_archive_block_storage_in_current_block(arc, pos)) {
        if (unityfs_archive_block_storage_seek_to_block(arc, pos)) {
            return 1;
        }
    }
    arc->curpos = pos;
    return 0;
}

int unityfs_archive_block_storage_seek(unityfs_archive* arc, int64_t offset, int origin) {
    if (!arc) return 1;
    int64_t pos = 0;
    if (origin == SEEK_SET) {
        pos = offset;
    } else if (origin == SEEK_CUR) {
        pos = arc->curpos + offset;
    } else if (origin == SEEK_END) {
        pos = arc->maxpos + offset;
    } else {
        return 1;
    }
    if (arc->curpos == pos) return 0;
    return unityfs_archive_block_storage_seek3(arc, pos);
}

size_t unityfs_archive_block_storage_read(unityfs_archive* arc, size_t buf_len, char* buf) {
    if (!arc || !buf) return 0;
    if (arc->curpos < 0 || arc->curpos >= arc->maxpos) return 0;
    if (!unityfs_archive_block_storage_in_current_block(arc, arc->curpos)) {
        if (unityfs_archive_block_storage_seek_to_block(arc, arc->curpos)) {
            return 0;
        }
    }
    if (!buf_len) return 0;
    int64_t opos = arc->curpos;
    size_t w = 0, tw = min(buf_len, arc->maxpos - opos);
    if (arc->current_block->d.compression) {
        if (!arc->curbuf) return 0;
        while (w < tw) {
            if (!unityfs_archive_block_storage_in_current_block(arc, arc->curpos)) {
                if (unityfs_archive_block_storage_seek_to_block(arc, arc->curpos)) {
                    return w;
                }
            }
            int64_t lastpos = arc->current_block_start + arc->current_block->d.uncompress_size;
            size_t nw = min(tw - w, lastpos - arc->curpos);
            memcpy(buf + w, arc->curbuf + arc->curpos - arc->current_block_start, nw);
            w += nw;
            arc->curpos += nw;
        }
    } else {
        while (w < tw) {
            if (!unityfs_archive_block_storage_in_current_block(arc, arc->curpos)) {
                if (unityfs_archive_block_storage_seek_to_block(arc, arc->curpos)) {
                    return w;
                }
            }
            int64_t npos = arc->basepos + arc->current_block_basepos + arc->curpos - arc->current_block_start;
            size_t nw = min(tw - w, arc->current_block_start + arc->current_block->d.uncompress_size - arc->curpos), temp = 0;
            if (fileop::ftell(arc->f) != npos) {
                if (fileop::fseek(arc->f, npos, SEEK_SET)) {
                    return w;
                }
            }
            if ((temp = fread(buf + w, 1, nw, arc->f)) != nw) {
                w += temp;
                arc->curpos += temp;
                return w;
            }
            w += nw;
            arc->curpos += nw;
        }
    }
    return w;
}

size_t unityfs_archive_block_storage_read2(void* arc, size_t buf_len, char* buf) {
    return unityfs_archive_block_storage_read((unityfs_archive*)arc, buf_len, buf);
}

int64_t unityfs_archive_block_storage_tell2(void* arc) {
    return unityfs_archive_block_storage_tell((unityfs_archive*)arc);
}

int unityfs_archive_block_storage_seek2(void* arc, int64_t offset, int origin) {
    return unityfs_archive_block_storage_seek((unityfs_archive*)arc, offset, origin);
}
