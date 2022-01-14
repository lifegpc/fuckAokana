#ifndef _UNITYFS_OBJECT_VECTOR_H
#define _UNITYFS_OBJECT_VECTOR_H
#include "file_reader.h"
#include "../unityfs_private.h"
typedef struct unityfs_vector_object {
    unityfs_object** obj;
    uint64_t size;
} unityfs_vector_object;
void free_unityfs_vector_object(unityfs_vector_object* obj);
void free_unityfs_object_vector(void* data);
bool is_vector_type(std::string name);
bool load_vector(unityfs_object* obj, unityfs_type_tree* tree, file_reader_file* f);
void print_vector(unityfs_object* obj, int indent, int indent_now);
size_t get_vector_count(unityfs_object* obj);
#endif
