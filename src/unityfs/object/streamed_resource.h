#ifndef _UNITYFS_OBJECT_STREAMED_RESOURCE_H
#define _UNITYFS_OBJECT_STREAMED_RESOURCE_H
#include "file_reader.h"
#include "../unityfs_private.h"
typedef struct unityfs_streamed_resource {
    int64_t basepos;
    int64_t curpos;
    int64_t size;
    file_reader_file* f;
} unityfs_streamed_resource;
bool is_streamed_resource_type(std::string name);
#endif
