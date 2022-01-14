#ifndef _UNITYFS_OBJECT_DICT_H
#define _UNITYFS_OBJECT_DICT_H
#include "file_reader.h"
#include "../unityfs_private.h"
void free_unityfs_object_dict(void* data);
bool load_dict(unityfs_object* obj, unityfs_type_tree* tree, file_reader_file* f);
void print_dict(unityfs_object* obj, int indent, int indent_now);
size_t get_dict_count(unityfs_object* obj);
#endif
