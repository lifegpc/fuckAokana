#ifndef _UNITYFS_OBJECT_H
#define _UNITYFS_OBJECT_H
#include "file_reader.h"
#include "unityfs_private.h"
#if __cplusplus
extern "C" {
#endif
unityfs_object* load_object(unityfs_type_tree* tree, file_reader_file* f);
void dump_unityfs_inline_object(unityfs_object* obj, int indent, int indent_now);
void print_unityfs_object_value(unityfs_object* obj, int indent, int indent_now);
#if __cplusplus
}
bool unityfs_object_is_inline_value(unityfs_object* obj);
#endif
#endif
