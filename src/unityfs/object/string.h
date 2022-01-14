#ifndef _UNITYFS_OBJECT_STRING_H
#define _UNITYFS_OBJECT_STRING_H
#include "file_reader.h"
#include "../unityfs_private.h"
bool is_string_type(std::string name);
bool load_string(unityfs_object* obj, file_reader_file* f, unityfs_type_tree* tree, bool& align);
void print_string(unityfs_object* obj, int indent, int indent_now);
#endif
