#ifndef _UNITYFS_UNITYFS_H
#define _UNITYFS_UNITYFS_H
#if __cplusplus
#include "../utils/dict.h"
#include <string>
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>
typedef struct unityfs_archive unityfs_archive;
typedef struct unityfs_environment unityfs_environment;
typedef struct unityfs_asset unityfs_asset;
typedef struct unityfs_object_info unityfs_object_info;
typedef struct unityfs_object unityfs_object;
typedef struct unityfs_streamed_resource unityfs_streamed_resource;
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
int unityfs_object_read_int64(unityfs_object* obj, int64_t* re);
int unityfs_object_read_uint64(unityfs_object* obj, uint64_t* re);
unityfs_object* unityfs_object_dict_get(unityfs_object* obj, const char* key);
unityfs_object* unityfs_object_pointer_resolve(unityfs_object* obj);
unityfs_streamed_resource* get_streamed_resource_from_object(unityfs_object* obj);
void free_unityfs_streamed_resource(unityfs_streamed_resource* res);
size_t unityfs_streamed_resource_read(unityfs_streamed_resource* res, size_t buf_len, char* buf);
int unityfs_streamed_resource_seek(unityfs_streamed_resource* res, int64_t offset, int origin);
int64_t unityfs_streamed_resource_tell(unityfs_streamed_resource* res);
#if __cplusplus
}
struct LinkedList<unityfs_asset*>* get_assets_from_archive(unityfs_archive* arc, bool exclude_resource = true);
bool unityfs_object_is_dict(unityfs_object* obj);
bool unityfs_object_is_map(unityfs_object* obj);
struct Dict<unityfs_object*, unityfs_object*>* unityfs_object_map_get(unityfs_object* obj);
template <typename ... Args>
bool unityfs_object_map_iter(unityfs_object* obj, bool(*callback)(unityfs_object* key, unityfs_object* value, Args...), Args... args) {
    auto d = unityfs_object_map_get(obj);
    if (!d) return false;
    return dict_iter(d, callback, false, args...);
}
bool unityfs_object_is_str(unityfs_object* obj);
bool unityfs_object_read_str(unityfs_object* obj, std::string& s);
bool unityfs_object_is_pointer(unityfs_object* obj);
bool unityfs_object_is_streamed_resource(unityfs_object* obj);
#endif
#endif
