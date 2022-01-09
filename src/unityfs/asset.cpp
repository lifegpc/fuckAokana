#include "asset.h"
#include "fuckaokana_config.h"

#include <malloc.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "assetref.h"
#include "block_storage.h"
#include "environment.h"
#include "file_reader.h"
#include "object_info.h"
#include "type.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

bool unityfs_asset_is_resource(std::string name) {
    auto le = name.length();
    if (le >= 5 && name.rfind(".resS") == (le - 5)) return true;
    if (le >= 9 && name.rfind(".resource") == (le - 9)) return true;
    return false;
}

void free_objects(struct dict_entry<int64_t, unityfs_object_info*> o) {
    free_unityfs_object_info(o.value);
}

void free_unityfs_asset(unityfs_asset* asset) {
    if (!asset) return;
    if (asset->tree) free_unityfs_type_metadata(asset->tree);
    dict_free(asset->objects, &free_objects);
    dict_free(asset->types);
    linked_list_clear(asset->adds);
    linked_list_clear(asset->asset_refs, &free_unityfs_assetref);
    free(asset);
}

unityfs_asset* create_asset_from_node(unityfs_archive* arc, unityfs_node_info* inf) {
    if (!arc || !inf) return nullptr;
    unityfs_asset* asset = (unityfs_asset*)malloc(sizeof(unityfs_asset));
    file_reader_file* f = nullptr;
    uint32_t num_objects;
    unityfs_assetref* ref = nullptr;
    if (!asset) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(asset, 0, sizeof(unityfs_asset));
    asset->info = inf;
    if (unityfs_asset_is_resource(inf->name)) {
        asset->is_resource = 1;
        return asset;
    }
    asset->endian = 1;
    if (unityfs_archive_block_storage_seek(arc, inf->offset, SEEK_SET)) {
        printf("Failed to seek in block storage.\n");
        goto end;
    }
    f = create_file_reader2((void*)arc, &unityfs_archive_block_storage_read2, &unityfs_archive_block_storage_seek2, &unityfs_archive_block_storage_tell2, asset->endian);
    if (!f) {
        printf("Out of memory.\n");
        goto end;
    }
    if (file_reader_read_uint32(f, &asset->metadata_size)) {
        printf("Can not read asset's metadata size.\n");
        goto end;
    }
    if (file_reader_read_uint32(f, &asset->file_size)) {
        printf("Can not read asset's file size.\n");
        goto end;
    }
    if (file_reader_read_uint32(f, &asset->format)) {
        printf("Can not read asset's format.\n");
        goto end;
    }
    if (file_reader_read_uint32(f, &asset->data_offset)) {
        printf("Can not read asset's data offset.\n");
        goto end;
    }
    uint32_t temp;
    if (asset->format >= 9) {
        if (file_reader_read_uint32(f, &temp)) {
            printf("Can not read asset's endian.\n");
            goto end;
        }
        asset->endian = temp ? 1 : 0;
        set_file_reader_endian(f, asset->endian);
    }
    asset->tree = create_metatree(f, asset->format);
    if (!asset->tree) {
        printf("Failed to load metadata tree.\n");
        goto end;
    }
    if (asset->format >= 7 && asset->format <= 13) {
        uint32_t tmp;
        if (file_reader_read_uint32(f, &tmp)) {
            printf("Can not read asset use long object ids or not.\n");
            goto end;
        }
        asset->long_object_ids = tmp ? 1 : 0;
    }
    if (file_reader_read_uint32(f, &num_objects)) {
        printf("Failed read count of objects in asset \"%s\".\n", asset->info->name);
        goto end;
    }
    for (uint32_t i = 0; i < num_objects; i++) {
        unityfs_object_info* obj = nullptr;
        if (asset->format >= 14) {
            if (file_reader_align(f)) {
                printf("Failed to align reader's position.\n");
                goto end;
            }
        }
        if (!(obj = create_object_info_from_asset(arc, asset))) {
            printf("Failed to create object info from asset \"%s\".\n", asset->info->name);
            goto end;
        }
        if (dict_have_key(asset->tree->type_trees, obj->type_id)) {
            struct dict_entry<int32_t, unityfs_type_tree*>* data = nullptr;
            if (!(data = dict_get(asset->tree->type_trees, obj->type_id))) {
                printf("Failed get type %" PRIi32 " from type tress in asset \"%s\".\n", obj->type_id, asset->info->name);
                free_unityfs_object_info(obj);
                goto end;
            }
            if (!dict_set(asset->types, obj->type_id, data->value)) {
                printf("Out of memory.\n");
                free_unityfs_object_info(obj);
                goto end;
            }
        } else if (!dict_have_key(asset->types, obj->type_id)) {
            auto trees = get_default_type_metadata(arc->env);
            if (!trees) {
                printf("Failed to get default type metadata.\n");
                free_unityfs_object_info(obj);
                goto end;
            }
            if (dict_have_key(trees->type_trees, obj->class_id)) {
                auto data = dict_get(trees->type_trees, obj->class_id);
                if (!data) {
                    printf("Failed get class %" PRIi32 " from default type tress.\n", obj->class_id);
                    free_unityfs_object_info(obj);
                    goto end;
                }
                if (!dict_set(asset->types, obj->type_id, data->value)) {
                    printf("Out of memory.\n");
                    free_unityfs_object_info(obj);
                    goto end;
                }
            } else {
                printf("class %" PRIi32 " absent from structs.dat.\n", obj->class_id);
                if (!dict_set<int32_t, unityfs_type_tree*>(asset->types, obj->type_id, nullptr)) {
                    printf("Out of memory.\n");
                    free_unityfs_object_info(obj);
                    goto end;
                }
            }
        }
        if (dict_have_key(asset->objects, obj->path_id)) {
            printf("Duplicate asset object: %" PRIi64 ".\n", obj->path_id);
            free_unityfs_object_info(obj);
            goto end;
        }
        if (!dict_set(asset->objects, obj->path_id, obj, &free_unityfs_object_info)) {
            printf("Out of memory.\n");
            free_unityfs_object_info(obj);
            goto end;
        }
    }
    if (asset->format >= 11) {
        uint32_t num_adds;
        if (file_reader_read_uint32(f, &num_adds)) {
            printf("Failed to get count of addition datas.\n");
            goto end;
        }
        for (uint32_t i = 0; i < num_adds; i++) {
            unityfs_asset_add add;
            if (asset->format >= 14) {
                if (file_reader_align(f)) {
                    printf("Failed to align reader's position.\n");
                    goto end;
                }
            }
            if (asset_read_id(asset, f, &add.id)) {
                printf("Failed to get addition datas.\n");
                goto end;
            }
            if (file_reader_read_int32(f, &add.data)) {
                printf("Failed to get addition datas.\n");
                goto end;
            }
            if (!linked_list_append(asset->adds, &add)) {
                printf("Out of memory.\n");
                goto end;
            }
        }
    }
    if (!(ref = create_assetref_from_asset(asset))) {
        printf("Failed to create assetref for asset itself.\n");
        goto end;
    }
    if (!linked_list_append(asset->asset_refs, &ref)) {
        printf("Out of memory.\n");
        free_unityfs_assetref(ref);
        goto end;
    }
    if (asset->format >= 6) {
        uint32_t num_refs;
        if (file_reader_read_uint32(f, &num_refs)) {
            printf("Failed to get count of asset references from asset \"%s\".\n", asset->info->name);
            goto end;
        }
        for (uint32_t i = 0; i < num_refs; i++) {
            if (!(ref = create_assetref_from_asset(asset, f))) {
                printf("Failed to create asset references from asset \"%s\".\n", asset->info->name);
                goto end;
            }
            if (!linked_list_append(asset->asset_refs, &ref)) {
                printf("Out of memory.\n");
                free_unityfs_assetref(ref);
                goto end;
            }
        }
    }
    if (file_reader_read_str(f, nullptr)) {
        goto end;
    }
    if (f) free_file_reader(f);
    return asset;
end:
    if (f) free_file_reader(f);
    free_unityfs_asset(asset);
    return nullptr;
}

int asset_read_id(unityfs_asset* asset, file_reader_file* f, int64_t* re) {
    if (!asset || !f || !re) return 1;
    if (asset->format >= 14) {
        return file_reader_read_int64(f, re);
    } else {
        int32_t tmp;
        if (file_reader_read_int32(f, &tmp)) {
            return 1;
        }
        *re = tmp;
        return 0;
    }
}
