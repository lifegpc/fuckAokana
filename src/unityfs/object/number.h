#ifndef _UNITYFS_OBJECT_NUMBER_H
#define _UNITYFS_OBJECT_NUMBER_H
#include "file_reader.h"
#include "../unityfs_private.h"
bool is_number_type(std::string name);
bool load_number(unityfs_object* obj, file_reader_file* f);
void print_number(unityfs_object* obj, int indent, int indent_now);
#endif
