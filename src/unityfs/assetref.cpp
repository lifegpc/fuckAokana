#include "assetref.h"
#include "fuckaokana_config.h"

#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "str_util.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void free_unityfs_assetref(unityfs_assetref* ref) {
    if (!ref) return;
    if (ref->asset_path) free(ref->asset_path);
    if (ref->file_path) free(ref->file_path);
    free(ref);
}

unityfs_assetref* create_assetref_from_asset(unityfs_asset* asset) {
    if (!asset) return nullptr;
    unityfs_assetref* ref = nullptr;
    if (!(ref = (unityfs_assetref*)malloc(sizeof(unityfs_assetref)))) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(ref, 0, sizeof(unityfs_assetref));
    ref->source = asset;
    ref->asset = asset;
    ref->asset_self = 1;
    return ref;
}

unityfs_assetref* create_assetref_from_asset(unityfs_asset* asset, file_reader_file* f) {
    if (!asset || !f) return nullptr;
    unityfs_assetref* ref = nullptr;
    if (!(ref = (unityfs_assetref*)malloc(sizeof(unityfs_assetref)))) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(ref, 0, sizeof(unityfs_assetref));
    ref->source = asset;
    if (file_reader_read_str(f, &ref->asset_path)) {
        printf("Failed to read asset reference's asset path.\n");
        goto end;
    }
    if (file_reader_read(f, 16, ref->guid) < 16) {
        printf("Failed to read asset reference's guid.\n");
        goto end;
    }
    if (file_reader_read_int32(f, &ref->type)) {
        printf("Failed to read asset reference's type.\n");
        goto end;
    }
    if (file_reader_read_str(f, &ref->file_path)) {
        printf("Failed to read asset reference's file path.\n");
        goto end;
    }
    return ref;
end:
    free_unityfs_assetref(ref);
    return nullptr;
}

void dump_unityfs_assetref(unityfs_assetref* ref, int indent, int indent_now) {
    if (!ref || ref->asset_self) return;
    std::string ind(indent_now, ' ');
    std::string tmp("(Unknown)");
    if (ref->asset_path) tmp = ref->asset_path;
    printf("%sAsset Path: %s\n", ind.c_str(), tmp.c_str());
    tmp = std::string(ref->guid, 16);
    tmp = str_util::str_hex(tmp);
    printf("%sGUID: %s\n", ind.c_str(), tmp.c_str());
    printf("%sType: %" PRIi32 "\n", ind.c_str(), ref->type);
    tmp = ref->file_path ? ref->file_path : "(Unknown)";
    printf("%sFile path: %s\n", ind.c_str(), tmp.c_str());
}
