#include "type.h"
#include "fuckaokana_config.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "block_storage.h"
#include "file_reader.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void free_unityfs_type_metadata(unityfs_type_metadata* meta) {
    if (!meta) return;
    linked_list_clear(meta->class_ids);
    if (meta->generator_version) free(meta->generator_version);
    free(meta);
}

unityfs_type_metadata* create_metatree_from_asset(unityfs_archive* arc, unityfs_asset* asset) {
    if (!arc || !asset) return nullptr;
    unityfs_type_metadata* tree = (unityfs_type_metadata*)malloc(sizeof(unityfs_type_metadata));
    file_reader_file* f = nullptr;
    if (!tree) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(tree, 0, sizeof(unityfs_type_metadata));
    tree->format = asset->format;
    f = create_file_reader2((void*)arc, &unityfs_archive_block_storage_read2, &unityfs_archive_block_storage_seek2, &unityfs_archive_block_storage_tell2, asset->endian);
    if (!f) {
        printf("Out of memory.\n");
        goto end;
    }
    if (file_reader_read_str(f, &tree->generator_version)) {
        printf("Can not read metadata tree's generator version.\n");
        goto end;
    }
    if (file_reader_read_uint32(f, &tree->target_platform)) {
        printf("Can not read metadata tree's target platform.\n");
        goto end;
    }
    if (f) free_file_reader(f);
    return tree;
end:
    if (f) free_file_reader(f);
    free_unityfs_type_metadata(tree);
    return nullptr;
}
