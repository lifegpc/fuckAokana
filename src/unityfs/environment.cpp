#include "environment.h"
#include "unityfs.h"
#include "fuckaokana_config.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "../embed_data.h"
#include "file_reader.h"
#include "memfile.h"
#include "type.h"

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
