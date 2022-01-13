#ifndef _UNITYFS_UNITYFS_H
#define _UNITYFS_UNITYFS_H
#if __cplusplus
#include "linked_list.h"
#include <stdint.h>
extern "C" {
#endif
typedef struct unityfs_archive unityfs_archive;
typedef struct unityfs_environment unityfs_environment;
typedef struct unityfs_asset unityfs_asset;
typedef struct unityfs_object_info unityfs_object_info;
typedef struct unityfs_object unityfs_object;
/**
 * @brief Detect if a file is an unity fs file.
 * @param f file name
 * @return 1 if is an unity file otherwise 0, if an error occured, -1 is returned.
*/
int is_unityfs_file(const char* f);
void free_unityfs_archive(unityfs_archive* arc);
unityfs_archive* open_unityfs_archive(unityfs_environment* env, const char* f);
unityfs_environment* create_unityfs_environment();
void free_unityfs_environment(unityfs_environment* env);
void dump_unityfs_archive(unityfs_archive* arc, int indent, int indent_now);
unityfs_object_info* get_object_from_asset_by_path_id(unityfs_asset* asset, int64_t path_id);
void dump_unityfs_object_info(unityfs_object_info* obj, int indent, int indent_now);
unityfs_object* create_object_from_object_info(unityfs_object_info* obj);
void free_unityfs_object(unityfs_object* obj);
void dump_unityfs_object(unityfs_object* obj, int indent, int indent_now);
#if __cplusplus
}
struct LinkedList<unityfs_asset*>* get_assets_from_archive(unityfs_archive* arc, bool exclude_resource = true);
#endif
#endif
