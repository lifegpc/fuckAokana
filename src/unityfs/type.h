#ifndef _UNITYFS_TYPE_H
#define _UNITYFS_TYPE_H
#include "file_reader.h"
#include "unityfs_private.h"
#if __cplusplus
extern "C" {
unityfs_type_tree* create_typetree(file_reader_file* f, uint32_t format);
unityfs_type_metadata* create_metatree(file_reader_file* f, uint32_t format);
#endif
#if __cplusplus
}
#endif
#endif
