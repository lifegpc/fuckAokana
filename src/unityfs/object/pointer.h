#ifndef _UNITYFS_OBJECT_POINTER_H
#define _UNITYFS_OBJECT_POINTER_H
#include "file_reader.h"
#include "../unityfs_private.h"
typedef struct unityfs_pointer_object {
    uint64_t file_id;
    int64_t path_id;
} unityfs_pointer_object;
bool is_pointer_type(std::string name);
bool load_pointer(unityfs_object* obj, unityfs_type_tree* tree, file_reader_file* f);
void print_pointer(unityfs_object* obj, int indent, int indent_now);
#endif
