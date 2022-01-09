#ifndef _UNITYFS_UNITYFS_PRIVATE_H
#define _UNITYFS_UNITYFS_PRIVATE_H
#include <stdint.h>
#include <stdio.h>
#include "linked_list.h"
#include "dict.h"
typedef struct unityfs_asset unityfs_asset;
typedef struct unityfs_environment unityfs_environment;
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
typedef struct unityfs_type_tree {
    uint32_t format;
    struct LinkedList<struct unityfs_type_tree*>* children;
    int32_t version;
    char* type;
    char* name;
    int32_t size;
    uint32_t index;
    int32_t flags;
    unsigned char is_array;
} unityfs_type_tree;
typedef struct unityfs_object_info {
    unityfs_asset* asset;
    int64_t path_id;
    int64_t data_offset;
    uint32_t size;
    int32_t type_id;
    int32_t class_id;
    unsigned char is_destroyed;
} unityfs_object_info;
typedef struct unityfs_type_metadata {
    uint32_t format;
    struct LinkedList<int32_t>* class_ids;
    char* generator_version;
    uint32_t target_platform;
    struct Dict<int32_t, char[32]>* hashes;
    unsigned char has_type_trees;
    int32_t num_types;
    struct Dict<int32_t, unityfs_type_tree*>* type_trees;
} unityfs_type_metadata;
typedef struct unityfs_asset_add {
    int64_t id;
    int32_t data;
} unityfs_asset_add;
typedef struct unityfs_assetref {
    unityfs_asset* source;
    unityfs_asset* asset;
    char* asset_path;
    char guid[16];
    int32_t type;
    char* file_path;
    /// 1 if don't reference another asset otherwise 0
    unsigned char asset_self;
} unityfs_assetref;
typedef struct unityfs_asset {
    unityfs_node_info* info;
    uint32_t metadata_size;
    uint32_t file_size;
    uint32_t format;
    uint32_t data_offset;
    unityfs_type_metadata* tree;
    struct Dict<int64_t, unityfs_object_info*>* objects;
    struct Dict<int32_t, unityfs_type_tree*>* types;
    struct LinkedList<unityfs_asset_add>* adds;
    struct LinkedList<unityfs_assetref*>* asset_refs;
    /// 0 if little endian otherwise big endian
    unsigned char endian : 1;
    unsigned char long_object_ids : 1;
    unsigned char is_resource : 1;
} unityfs_asset;
typedef struct unityfs_archive {
    unityfs_environment* env;
    char* name;
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
    struct LinkedList<unityfs_asset*>* assets;
    /// The start position in true file of storage blocks
    int64_t basepos;
    /// The size of uncompressed data in storage blocks
    int64_t maxpos;
    /// The current position of block storages
    int64_t curpos;
    struct LinkedList<unityfs_block_info>* current_block;
    /// The start position of current block (uncompressed size) in block storages
    int64_t current_block_start;
    /// The start position of current block in compressed size
    int64_t current_block_basepos;
    /// The undecompressed data for current block
    char* curbuf;
    unsigned char compression : 6;
    unsigned char eof_metadata : 1;
    /// 0 if little endian otherwise big endian
    unsigned char endian : 1;
} unityfs_archive;
typedef struct unityfs_environment {
    struct LinkedList<unityfs_archive*>* archives;
    unityfs_type_metadata* meta;
} unityfs_environment;
void free_unityfs_node_info(unityfs_node_info n);
void free_unityfs_asset(unityfs_asset* asset);
void free_unityfs_type_tree(unityfs_type_tree* tree);
void free_unityfs_type_metadata(unityfs_type_metadata* meta);
void free_unityfs_object_info(unityfs_object_info* obj);
void free_unityfs_assetref(unityfs_assetref* ref);
#define UNITYFS_COMPRESSION_NONE 0
#define UNITYFS_COMPRESSION_LZMA 1
#define UNITYFS_COMPRESSION_LZ4 2
#define UNITYFS_COMPRESSION_LZ4HC 3
#define UNITYFS_COMPRESSION_LZHAM 4
#endif
