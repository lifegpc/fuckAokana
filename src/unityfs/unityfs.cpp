#include "unityfs.h"
#include "unityfs_private.h"
#include "fuckaokana_config.h"

#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "asset.h"
#include "cstr_util.h"
#include "decompress.h"
#include "err.h"
#include "file_reader.h"
#include "fileop.h"

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

#if HAVE_PRINTF_S
#define printf printf_s
#endif

int is_unityfs_file(const char* f) {
    if (!f) return -1;
    if (!fileop::exists(f)) {
        printf("File \"%s\" not exists.\n", f);
        return -1;
    }
    int fd = 0, err = 0, re = 0;
    FILE* s = nullptr;
    char buf[8];
    if ((err = fileop::open(f, fd, O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD))) {
        std::string errmsg;
        if (!err::get_errno_message(errmsg, err)) {
            errmsg = "Unknown error.";
        }
        printf("Can not open file \"%s\": %s.\n", f, errmsg.c_str());
        return -1;
    }
    if (!(s = fileop::fdopen(fd, "r"))) {
        printf("Can not open file \"%s\".\n", f);
        fileop::close(fd);
        return -1;
    }
    if (fread(buf, 1, 8, s) < 8) {
        goto end;
    }
    cstr_tolowercase2(buf, 8);
    re = (!strncmp(buf, "unityfs", 8)) ? 1 : 0;
end:
    if (s) fileop::fclose(s);
    return re;
}

void free_unityfs_node_info(unityfs_node_info n) {
    if (n.name) free(n.name);
}

void free_unityfs_archive(unityfs_archive* arc) {
    if (!arc) return;
    if (arc->generator_version) free(arc->generator_version);
    if (arc->unity_version) free(arc->unity_version);
    if (arc->f) fileop::fclose(arc->f);
    linked_list_clear(arc->blocks);
    linked_list_clear(arc->nodes, &free_unityfs_node_info);
    linked_list_clear(arc->assets, &free_unityfs_asset);
    if (arc->curbuf) free(arc->curbuf);
    free(arc);
}

unityfs_archive* open_unityfs_archive(const char* f) {
    if (!f) return nullptr;
    if (!fileop::exists(f)) {
        printf("File \"%s\" not exists.\n", f);
        return nullptr;
    }
    int fd = 0, err = 0;
    unityfs_archive* arc = nullptr;
    file_reader_file* r = nullptr;
    char* sign = nullptr, * obuf = nullptr, * dbuf = nullptr, * buf = nullptr;
    arc = (unityfs_archive*)malloc(sizeof(unityfs_archive));
    if (!arc) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(arc, 0, sizeof(unityfs_archive));
    if ((err = fileop::open(f, fd, O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD))) {
        std::string errmsg;
        if (!err::get_errno_message(errmsg, err)) {
            errmsg = "Unknown error.";
        }
        printf("Can not open file \"%s\": %s.\n", f, errmsg.c_str());
        goto end;
    }
    if (!(arc->f = fileop::fdopen(fd, "r"))) {
        printf("Can not open file \"%s\".\n", f);
        fileop::close(fd);
        goto end;
    }
    arc->endian = 1;
    if (!(r = create_file_reader(arc->f, arc->endian))) {
        printf("Out of memory.\n");
        goto end;
    }
    if (file_reader_read_str(r, &sign)) {
        printf("Failed to read signature from file \"%s\".\n", f);
        goto end;
    }
    cstr_tolowercase2(sign, 0);
    if (strlen(sign) < 7 || strncmp(sign, "unityfs", 8) != 0) {
        printf("File \"%s\" is not a unityfs file.\n", f);
        free(sign);
        sign = nullptr;
        goto end;
    }
    free(sign);
    sign = nullptr;
    if (file_reader_read_int32(r, &arc->format_version)) {
        printf("Failed to read format version of file \"%s\".\n", f);
        goto end;
    }
    if (file_reader_read_str(r, &arc->unity_version)) {
        printf("Failed to read unity version of file \"%s\".\n", f);
        goto end;
    }
    if (file_reader_read_str(r, &arc->generator_version)) {
        printf("Failed to read generator version of file \"%s\".\n", f);
        goto end;
    }
    if (file_reader_read_int64(r, &arc->file_size)) {
        printf("Failed to read file size of file \"%s\".\n", f);
        goto end;
    }
    if (file_reader_read_uint32(r, &arc->compress_block_size)) {
        printf("Failed to read compressed block size of file \"%s\".\n", f);
        goto end;
    }
    if (file_reader_read_uint32(r, &arc->uncompress_block_size)) {
        printf("Failed to read uncompressed block size of file \"%s\".\n", f);
        goto end;
    }
    if (file_reader_read_uint32(r, &arc->flags)) {
        printf("Failed to read flags of file \"%s\".\n", f);
        goto end;
    }
    arc->compression = arc->flags & 0x3F;
    arc->eof_metadata = arc->flags & 0x80 ? 1 : 0;
    obuf = (char*)malloc(arc->compress_block_size);
    if (!obuf) {
        printf("Out of memory.\n");
        goto end;
    }
    if (arc->eof_metadata) {
        int64_t offset = fileop::ftell(arc->f);
        if (offset == -1) {
            printf("Can not get current position of file.\n");
            goto end;
        }
        if (fileop::fseek(arc->f, -arc->compress_block_size, SEEK_END)) {
            printf("Failed to seek file.\n");
            goto end;
        }
        if (fread(obuf, 1, arc->compress_block_size, arc->f) != arc->compress_block_size) {
            printf("Failed to read metadata from file \"%s\".\n", f);
            goto end;
        }
        if (fileop::fseek(arc->f, offset, SEEK_SET)) {
            printf("Failed to seek file.\n");
            goto end;
        }
    } else {
        if (fread(obuf, 1, arc->compress_block_size, arc->f) != arc->compress_block_size) {
            printf("Failed to read metadata from file \"%s\".\n", f);
            goto end;
        }
    }
    arc->basepos = fileop::ftell(arc->f);
    if (arc->basepos == -1) {
        printf("Failed to get current position of file \"%s\".\n", f);
        goto end;
    }
    if (!arc->compression) {
        buf = obuf;
    } else {
        if (decompress(obuf, &dbuf, arc->compression, arc->compress_block_size, &arc->uncompress_block_size)) {
            printf("Failed to decompress metadata.\n");
            goto end;
        }
        buf = dbuf;
    }
    uint32_t meta_size = arc->compression ? arc->uncompress_block_size : arc->compress_block_size, pos = 20;
    if (meta_size < 24) {
        printf("Metadata block is too small.\n");
        goto end;
    }
    memcpy(arc->guid, buf, 16);
    arc->num_blocks = cstr_read_int32((uint8_t*)buf + 16, arc->endian);
    if (arc->num_blocks <= 0) {
        printf("No storage blocks in file \"%s\".\n", f);
        goto end;
    }
    if (meta_size < 20 + 10 * arc->num_blocks + 4) {
        printf("Metadata block is too small.\n");
        goto end;
    }
    unityfs_block_info bk;
    for (int32_t i = 0; i < arc->num_blocks; i++) {
        bk.uncompress_size = cstr_read_int32((uint8_t*)buf + pos, arc->endian);
        pos += 4;
        arc->maxpos += bk.uncompress_size;
        bk.compress_size = cstr_read_int32((uint8_t*)buf + pos, arc->endian);
        pos += 4;
        bk.flags = cstr_read_int16((uint8_t*)buf + pos, arc->endian);
        pos += 2;
        bk.compression = bk.flags & 0x3F;
        if (!linked_list_append(arc->blocks, &bk)) {
            printf("Out of memory.\n");
            goto end;
        }
    }
    arc->num_nodes = cstr_read_int32((uint8_t*)buf + pos, arc->endian);
    pos += 4;
    if (arc->num_nodes <= 0) {
        printf("No nodes in file \"%s\".\n", f);
        goto end;
    }
    unityfs_node_info ni;
    memset(&ni, 0, sizeof(unityfs_node_info));
    for (int32_t i = 0; i < arc->num_nodes; i++) {
        if (pos + 20 > meta_size) {
            printf("Metadata block is too small.\n");
            goto end;
        }
        ni.offset = cstr_read_int64((uint8_t*)buf + pos, arc->endian);
        pos += 8;
        ni.size = cstr_read_int64((uint8_t*)buf + pos, arc->endian);
        pos += 8;
        ni.status = cstr_read_int32((uint8_t*)buf + pos, arc->endian);
        pos += 4;
        if (pos > meta_size) {
            printf("Metadata block is too small.\n");
            goto end;
        }
        size_t tpos = pos;
        if (cstr_read_str(buf, &ni.name, &tpos, meta_size)) {
            printf("Failed to get node name from file \"%s\".\n", f);
            goto end;
        }
        pos = tpos;
        if (!linked_list_append(arc->nodes, &ni)) {
            free_unityfs_node_info(ni);
            printf("Out of memory.\n");
            goto end;
        }
        memset(&ni, 0, sizeof(unityfs_node_info));
    }
    unityfs_asset* asset = nullptr;
    auto node = arc->nodes;
    for (int32_t i = 0; i < arc->num_nodes; i++) {
        if (!(asset = create_asset_from_node(arc, &node->d))) {
            printf("Failed to load asset in file \"%s\".\n", f);
            goto end;
        }
        if (!linked_list_append(arc->assets, &asset)) {
            printf("Out of memory.\n");
            free_unityfs_asset(asset);
            goto end;
        }
        node = node->next;
    }
    if (r) free_file_reader(r);
    if (obuf) free(obuf);
    if (dbuf) free(dbuf);
    return arc;
end:
    if (r) free_file_reader(r);
    if (obuf) free(obuf);
    if (dbuf) free(dbuf);
    if (arc) free_unityfs_archive(arc);
    return nullptr;
}
