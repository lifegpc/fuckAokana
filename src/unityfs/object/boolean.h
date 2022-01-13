#ifndef _UNITYFS_OBJECT_BOOLEAN_H
#define _UNITYFS_OBJECT_BOOLEAN_H
#include "file_reader.h"
#include "../unityfs_private.h"
bool is_bool_type(std::string name);
bool load_boolean(unityfs_object* obj, file_reader_file* f);
void print_boolean(unityfs_object* obj, int indent, int indent_now);
#endif
