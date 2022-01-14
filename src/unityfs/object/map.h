#ifndef _UNITYFS_OBJECT_MAP_H
#define _UNITYFS_OBJECT_MAP_H
#include "file_reader.h"
#include "../unityfs_private.h"
void free_unityfs_object_map(void* data);
bool is_map_type(std::string name);
bool load_map(unityfs_object* obj, unityfs_type_tree* tree, file_reader_file* f);
void print_map(unityfs_object* obj, int indent, int indent_now);
size_t get_map_count(unityfs_object* obj);
#endif
