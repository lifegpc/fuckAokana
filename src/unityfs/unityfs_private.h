#ifndef _UNITYFS_UNITYFS_PRIVATE_H
#define _UNITYFS_UNITYFS_PRIVATE_H
#include <stdint.h>
#include <stdio.h>
#include "linked_list.h"
typedef struct unityfs_block_info {
    int32_t compress_size;
    int32_t uncompress_size;
    int16_t flags;
    unsigned char compression;
} unityfs_block_info;
typedef struct unityfs_node_info {
    int64_t offset;
    int64_t size;
    int32_t status;
    char* name;
} unityfs_node_info;
typedef struct unityfs_archive {
    int32_t format_version;
    char* unity_version;
    char* generator_version;
    int64_t file_size;
    uint32_t compress_block_size;
    uint32_t uncompress_block_size;
    uint32_t flags;
    FILE* f;
    char guid[16];
    struct LinkedList<unityfs_block_info>* blocks;
    int32_t num_blocks;
    struct LinkedList<unityfs_node_info>* nodes;
    int32_t num_nodes;
    /// The start position in true file of storage blocks
    int64_t basepos;
    /// The size of uncompressed data in storage blocks
    int64_t maxpos;
    unsigned char compression : 6;
    unsigned char eof_metadata : 1;
    /// 0 if little endian otherwise big endian
    unsigned char endian : 1;
} unityfs_archive;
void free_unityfs_node_info(unityfs_node_info n);
#define UNITYFS_COMPRESSION_NONE 0
#define UNITYFS_COMPRESSION_LZMA 1
#define UNITYFS_COMPRESSION_LZ4 2
#define UNITYFS_COMPRESSION_LZ4HC 3
#define UNITYFS_COMPRESSION_LZHAM 4
#endif
