#include "object_info.h"
#include "fuckaokana_config.h"

#include <malloc.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "asset.h"
#include "block_storage.h"
#include "environment.h"
#include "file_reader.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void free_unityfs_object_info(unityfs_object_info* obj) {
    if (!obj) return;
    free(obj);
}

unityfs_object_info* create_object_info_from_asset(unityfs_archive* arc, unityfs_asset* asset) {
    if (!arc || !asset) return nullptr;
    unityfs_object_info* obj = nullptr;
    file_reader_file* f = nullptr;
    uint32_t tmp;
    obj = (unityfs_object_info*)malloc(sizeof(unityfs_object_info));
    if (!obj) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(obj, 0, sizeof(unityfs_object_info));
    obj->asset = asset;
    if (!(f = create_file_reader2((void*)arc, &unityfs_archive_block_storage_read2, &unityfs_archive_block_storage_seek2, &unityfs_archive_block_storage_tell2, asset->endian))) {
        printf("Out of memory.\n");
        goto end;
    }
    if (asset->long_object_ids) {
        if (file_reader_read_int64(f, &obj->path_id)) {
            printf("Failed to read object's path id from asset \"%s\".\n", asset->info->name);
            goto end;
        }
    } else {
        if (asset_read_id(asset, f, &obj->path_id)) {
            printf("Failed to read object's path id from asset \"%s\".\n", asset->info->name);
            goto end;
        }
    }
    if (file_reader_read_uint32(f, &tmp)) {
        printf("Failed to read object's data offset from asset \"%s\".\n", asset->info->name);
        goto end;
    }
    obj->data_offset = (int64_t)tmp + asset->data_offset;
    if (file_reader_read_uint32(f, &obj->size)) {
        printf("Failed to read object's size from asset \"%s\".\n", asset->info->name);
        goto end;
    }
    if (asset->format < 17) {
        int16_t tmp;
        if (file_reader_read_int32(f, &obj->type_id)) {
            printf("Failed to read object's type id from asset \"%s\".\n", asset->info->name);
            goto end;
        }
        if (file_reader_read_int16(f, &tmp)) {
            printf("Failed to read object's class id from asset \"%s\".\n", asset->info->name);
            goto end;
        }
        obj->class_id = tmp;
    } else {
        int32_t type_id;
        LinkedList<int32_t>* class_id;
        if (file_reader_read_int32(f, &type_id)) {
            printf("Failed to read object's type id from asset \"%s\".\n", asset->info->name);
            goto end;
        }
        class_id = linked_list_get(asset->tree->class_ids, type_id);
        if (!class_id) {
            printf("Failed to read object's class id from asset \"%s\": index %" PRIi32 " out of range.\n", asset->info->name, type_id);
            goto end;
        }
        obj->type_id = class_id->d;
        obj->class_id = class_id->d;
    }
    if (asset->format <= 10) {
        int16_t tmp;
        if (file_reader_read_int16(f, &tmp)) {
            printf("Failed to read object's destoried information from asset \"%s\".\n", asset->info->name);
            goto end;
        }
        obj->is_destroyed = tmp ? 1 : 0;
    }
    if (asset->format >= 11 && asset->format <= 16) {
        if (file_reader_read_int16(f, NULL)) {
            goto end;
        }
    }
    if (asset->format >= 15 && asset->format <= 16) {
        if (file_reader_read_char(f, NULL)) {
            goto end;
        }
    }
    if (f) free_file_reader(f);
    return obj;
end:
    if (f) free_file_reader(f);
    free_unityfs_object_info(obj);
    return nullptr;
}

void dump_unityfs_object_info(unityfs_object_info* obj, int indent, int indent_now) {
    if (!obj) return;
    std::string ind(indent_now, ' ');
    printf("%sPath ID: %" PRIi64 "\n", ind.c_str(), obj->path_id);
    printf("%sData offset: %" PRIi64 "\n", ind.c_str(), obj->data_offset);
    printf("%sSize: %" PRIu32 "\n", ind.c_str(), obj->size);
    printf("%sType ID: %" PRIi32 "\n", ind.c_str(), obj->type_id);
    printf("%sClass ID: %" PRIi32 "\n", ind.c_str(), obj->class_id);
    std::string tmp = obj->is_destroyed ? "yes" : "no";
    printf("%sIs destroyed: %s\n", ind.c_str(), tmp.c_str());
    if (obj->asset) {
        if (obj->asset->info && obj->asset->info->name) {
            printf("%sFrom asset: %s\n", ind.c_str(), obj->asset->info->name);
        }
    }
}

unityfs_type_tree* get_object_info_tree(unityfs_object_info* obj) {
    if (!obj) return nullptr;
    if (obj->type_id >= 0) {
        return dict_get_value(obj->asset->types, obj->type_id);
    } else {
        if (dict_have_key(obj->asset->tree->type_trees, obj->type_id)) {
            return dict_get_value(obj->asset->tree->type_trees, obj->type_id);
        } else if (dict_have_key(obj->asset->tree->type_trees, obj->class_id)) {
            return dict_get_value(obj->asset->tree->type_trees, obj->class_id);
        } else {
            unityfs_type_metadata* meta = nullptr;
            if (obj->asset->arc) {
                meta = get_default_type_metadata(obj->asset->arc->env);
            }
            if (meta) {
                return dict_get_value(meta->type_trees, obj->class_id);
            }
        }
    }
    return nullptr;
}
