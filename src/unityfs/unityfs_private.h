#ifndef _UNITYFS_UNITYFS_PRIVATE_H
#define _UNITYFS_UNITYFS_PRIVATE_H
#include <stdint.h>
#include <stdio.h>
#include <string>
#include "fuckaokana_config.h"
#include "linked_list.h"
#include "dict.h"
#include "unityfs.h"
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
typedef struct unityfs_object {
    void* data;
    char* type;
    unsigned char is_number : 1;
    unsigned char is_bool : 1;
} unityfs_object;
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
    unityfs_archive* arc;
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
void dump_unityfs_block_info(unityfs_block_info bk, int indent, int indent_now);
void dump_unityfs_node_info(unityfs_node_info n, int indent, int indent_now);
void dump_unityfs_asset(unityfs_asset* asset, int indent, int indent_now);
void dump_unityfs_type_metadata(unityfs_type_metadata* meta, int indent, int indent_now);
void dump_unityfs_type_tree(unityfs_type_tree* tree, int indent, int indent_now);
void dump_unityfs_asset_add(unityfs_asset_add add, int indent, int indent_now);
void dump_unityfs_assetref(unityfs_assetref* ref, int indent, int indent_now);
#if HAVE_PRINTF_S
#define printf printf_s
#endif
template <typename T, typename ... Args>
void dump_list(size_t index, T data, const char* name, int indent, int indent_now, void(*callback)(T data, int indent, int indent_now, Args... args), Args... args) {
    std::string ind(indent_now, ' ');
    std::string tmp("Index");
    if (name) tmp = name;
    printf("%s%s %zi:\n", ind.c_str(), tmp.c_str(), index);
    if (callback) callback(data, indent, indent_now + indent, args...);
}
template <typename T, typename ... Args>
void dump_simple_list(struct LinkedList<T>* li, const char* name, void(*callback)(T data, int indent, int indent_now, Args... args), int indent, int indent_now, size_t max_inline_count, Args... args) {
    if (!callback || !name) return;
    std::string ind(indent_now, ' ');
    size_t c = linked_list_count(li);
    if (c == 0) {
        printf("%s%s: (Empty)\n", ind.c_str(), name);
    } else if (c <= max_inline_count) {
        printf("%s%s: ", ind.c_str(), name);
        struct LinkedList<T>* t = linked_list_head(li);
        callback(t->d, indent, 0, args...);
        while (t->next) {
            t = t->next;
            printf(", ");
            callback(t->d, indent, 0, args...);
        }
        printf("\n");
    } else {
        printf("%s%s: \n", ind.c_str(), name);
        struct LinkedList<T>* t = linked_list_head(li);
        callback(t->d, indent, indent_now + indent, args...);
        printf("\n");
        while (t->next) {
            t = t->next;
            callback(t->d, indent, indent_now + indent, args...);
            printf("\n");
        }
    }
}
template <typename K, typename V, typename ... Args>
void dump_dict(K key, V value, const char* name, int indent, int indent_now, void(*key_callback)(K key, int indent, int indent_now), void(*value_callback)(V value, int indent, int indent_now, Args... args), Args... args) {
    if (!key_callback || !value_callback) return;
    std::string ind(indent_now, ' ');
    std::string tmp("Key");
    if (name) tmp = name;
    printf("%s%s ", ind.c_str(), tmp.c_str());
    key_callback(key, indent, 0);
    printf(": \n");
    value_callback(value, indent, indent_now + indent, args...);
}
#ifdef HAVE_PRINTF_S
#undef printf
#endif
#define UNITYFS_COMPRESSION_NONE 0
#define UNITYFS_COMPRESSION_LZMA 1
#define UNITYFS_COMPRESSION_LZ4 2
#define UNITYFS_COMPRESSION_LZ4HC 3
#define UNITYFS_COMPRESSION_LZHAM 4
#define UNITYFS_TYPE_TREE_POST_ALIGN 0x4000
#endif
