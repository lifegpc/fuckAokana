#include "asset.h"
#include "fuckaokana_config.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "block_storage.h"
#include "file_reader.h"
#include "type.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void free_unityfs_asset(unityfs_asset* asset) {
    if (!asset) return;
    if (asset->tree) free_unityfs_type_metadata(asset->tree);
    free(asset);
}

unityfs_asset* create_asset_from_node(unityfs_archive* arc, unityfs_node_info* inf) {
    if (!arc || !inf) return nullptr;
    unityfs_asset* asset = (unityfs_asset*)malloc(sizeof(unityfs_asset));
    file_reader_file* f = nullptr;
    if (!asset) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(asset, 0, sizeof(unityfs_asset));
    asset->info = inf;
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
    asset->tree = create_metatree_from_asset(arc, asset);
    if (!asset->tree) {
        printf("Failed to load metadata tree.\n");
        goto end;
    }
    if (f) free_file_reader(f);
    return asset;
end:
    if (f) free_file_reader(f);
    free_unityfs_asset(asset);
    return nullptr;
}
