#include "streamed_resource.h"

#include <malloc.h>
#include <string.h>
#include "../asset.h"
#include "../block_storage.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

bool is_streamed_resource_type(std::string name) {
    return name == "StreamedResource";
}

bool unityfs_object_is_streamed_resource(unityfs_object* obj) {
    if (!obj || !obj->type) return false;
    return is_streamed_resource_type(obj->type);
}

void free_unityfs_streamed_resource(unityfs_streamed_resource* res) {
    if (!res) return;
    if (res->f) free_file_reader(res->f);
    free(res);
}

unityfs_streamed_resource* get_streamed_resource_from_object(unityfs_object* obj) {
    if (!obj || !obj->type || !obj->data || !is_streamed_resource_type(obj->type)) return nullptr;
    auto ofs = unityfs_object_dict_get(obj, "m_Offset");
    auto si = unityfs_object_dict_get(obj, "m_Size");
    auto sc = unityfs_object_dict_get(obj, "m_Source");
    int64_t offset = 0;
    std::string source;
    unityfs_asset* asset = nullptr;
    if (!ofs || !si || !sc) return nullptr;
    auto r = (unityfs_streamed_resource*)malloc(sizeof(unityfs_streamed_resource));
    if (!r) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(r, 0, sizeof(unityfs_streamed_resource));
    if (unityfs_object_read_int64(ofs, &offset)) {
        printf("Failed to offset from object.\nThe dump of object: \n");
        dump_unityfs_object(ofs, 2, 2);
        goto end;
    }
    if (unityfs_object_read_int64(si, &r->size)) {
        printf("Failed to size from object.\nThe dump of object: \n");
        dump_unityfs_object(si, 2, 2);
        goto end;
    }
    if (!unityfs_object_read_str(sc, source)) {
        printf("Failed to source from object.\nThe dump of object: \n");
        dump_unityfs_object(sc, 2, 2);
        goto end;
    }
    if (!(asset = unityfs_asset_get_asset(obj->asset, source.c_str()))) {
        printf("Failed to get asset %s.\n", source.c_str());
        goto end;
    }
    if (asset->arc && asset->info) {
        r->basepos = offset + asset->info->offset;
        r->f = create_file_reader2((void*)asset->arc, &unityfs_archive_block_storage_read2, &unityfs_archive_block_storage_seek2, &unityfs_archive_block_storage_tell2, asset->endian);
        if (!r->f) {
            printf("Out of memory.\n");
            goto end;
        }
    }
    if (!r->f) {
        printf("Don't know how to get data from asset.\n");
        goto end;
    }
    return r;
end:
    if (r) free_unityfs_streamed_resource(r);
    return nullptr;
}

size_t unityfs_streamed_resource_read(unityfs_streamed_resource* res, size_t buf_len, char* buf) {
    if (!res || !res->f || !buf) return 0;
    if (res->curpos >= res->size) return 0;
    size_t nw = min(res->size - res->curpos, buf_len);
    int64_t ofs = res->curpos + res->basepos, nofs = 0;
    if ((nofs = file_reader_tell(res->f)) < 0) {
        return 0;
    }
    if (ofs != nofs) {
        if (file_reader_seek(res->f, ofs, SEEK_SET)) {
            return 0;
        }
    }
    size_t w = file_reader_read(res->f, nw, buf);
    res->curpos += w;
    return w;
}

int unityfs_streamed_resource_seek(unityfs_streamed_resource* res, int64_t pos) {
    if (!res || !res->f) return 1;
    if (pos < 0 || pos > res->size) return 1;
    res->curpos = pos;
    return 0;
}

int unityfs_streamed_resource_seek(unityfs_streamed_resource* res, int64_t offset, int origin) {
    if (!res || !res->f) return 1;
    int64_t pos = 0;
    if (origin == SEEK_SET) {
        pos = offset;
    } else if (origin == SEEK_CUR) {
        pos = res->curpos + offset;
    } else if (origin == SEEK_END) {
        pos = res->size + offset;
    } else {
        return 1;
    }
    if (res->curpos == pos) return 0;
    return unityfs_streamed_resource_seek(res, pos);
}

int64_t unityfs_streamed_resource_tell(unityfs_streamed_resource* res) {
    if (!res || !res->f) return -1;
    return res->curpos;
}
