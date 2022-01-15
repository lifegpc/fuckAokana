#include "environment.h"
#include "unityfs.h"
#include "fuckaokana_config.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "cstr_util.h"
#include "../embed_data.h"
#include "file_reader.h"
#include "memfile.h"
#include "type.h"
#include "urlparse.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

unityfs_environment* create_unityfs_environment() {
    unityfs_environment* env = nullptr;
    env = (unityfs_environment*)malloc(sizeof(unityfs_environment));
    if (!env) return nullptr;
    memset(env, 0, sizeof(unityfs_environment));
    return env;
}

unityfs_type_metadata* get_default_type_metadata(unityfs_environment* env) {
    if (!env) return nullptr;
    if (env->meta) return env->meta;
    CMemFile* mf = nullptr;
    file_reader_file* f = nullptr;
    unityfs_type_metadata* meta = nullptr;
    if (!(mf = new_cmemfile((const char*)get_structs_data(), get_structs_size()))) {
        printf("Out of memory.\n");
        return nullptr;
    }
    if (!(f = create_file_reader2((void*)mf, &cmemfile_read2, &cmemfile_seek2, &cmemfile_tell2, 0))) {
        printf("Out of memory.\n");
        goto end;
    }
    if (!(meta = create_metatree(f, 15))) {
        printf("Failed to create default type metadata.\n");
        goto end;
    }
    env->meta = meta;
end:
    if (mf) free_cmemfile(mf);
    if (f) free_file_reader(f);
    return meta;
}

void free_unityfs_environment(unityfs_environment* env) {
    if (!env) return;
    linked_list_clear(env->archives, &free_unityfs_archive);
    if (env->meta) free_unityfs_type_metadata(env->meta);
    free(env);
}

int add_archive_to_environment(unityfs_environment* env, unityfs_archive* arc) {
    if (!env || !arc) return 1;
    return linked_list_append(env->archives, &arc) ? 0 : 1;
}

int delete_archive_from_environment(unityfs_environment* env, unityfs_archive* arc) {
    if (!env || !arc) return 1;
    if (!linked_list_have(env->archives, arc)) return 0;
    return linked_list_delete(env->archives, arc) ? 0 : 1;
}

bool compare_archive_key(unityfs_archive* arc, const char* name) {
    if (!arc->name || !name) return false;
    return !cstr_stricmp(arc->name, name);
}

bool compare_asset_name(unityfs_asset* asset, const char* name) {
    if (!name) return false;
    if (asset->info && asset->info->name) return !cstr_stricmp(asset->info->name, name);
    return false;
}

unityfs_asset* unityfs_environment_get_asset(unityfs_environment* env, const char* url) {
    if (!env || !url) return nullptr;
    auto u = urlparse(url, nullptr, 1);
    std::string archive, name, path;
    struct LinkedList<unityfs_asset*>* asset = nullptr;
    struct LinkedList<unityfs_archive*>* arc = nullptr;
    size_t i = 0;
    if (!u) {
        printf("Out of memory.\n");
        return nullptr;
    }
    if (strcmp(u->scheme, "archive")) {
        printf("Unsupported scheme %s. (Url: %s)\n", u->scheme, url);
        goto end;
    }
    path = u->path;
    if (path.find('/') == 0) {
        path = path.substr(1);
    }
    i = path.find('/');
    if (i == -1) {
        printf("Can not find archive name in path %s. (Url: %s)\n", u->path, url);
        goto end;
    }
    archive = path.substr(0, i);
    name = path.substr(i + 1);
    arc = linked_list_get(env->archives, archive.c_str(), &compare_archive_key);
    if (!arc || !arc->d) {
        printf("Failed to found archive %s. (Url: %s)\n", archive.c_str(), url);
        goto end;
    }
    asset = linked_list_get(arc->d->assets, name.c_str(), &compare_asset_name);
    if (!asset || !asset->d) {
        printf("Failed to found asset %s in archive %s. (Url: %s)\n", name.c_str(), archive.c_str(), url);
        goto end;
    }
    if (u) free_url_parse_result(u);
    return asset->d;
end:
    if (u) free_url_parse_result(u);
    return nullptr;
}

unityfs_asset* unityfs_environment_get_asset_by_filename(unityfs_environment* env, const char* path) {
    if (!env || !path) return nullptr;
    printf("Load asset with file name is not implemented.\n");
    return nullptr;
}
