#include "type.h"
#include "fuckaokana_config.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "block_storage.h"
#include "cstr_util.h"
#include "../embed_data.h"
#include "file_reader.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

int get_string(char* buf, uint32_t buf_size, int32_t offset, char** re) {
    if (!buf || !re) return 1;
    if (offset < 0) {
        offset &= 0x7fffffff;
        auto data = get_strings_data();
        if (!data) return 1;
        size_t ofs = offset;
        return cstr_read_str((char*)data, re, &ofs, get_strings_size());
    } else {
        size_t ofs = offset;
        return cstr_read_str(buf, re, &ofs, buf_size);
    }
}

void free_unityfs_type_tree(unityfs_type_tree* tree) {
    if (!tree) return;
    linked_list_clear(tree->children, &free_unityfs_type_tree);
    if (tree->type) free(tree->type);
    if (tree->name) free(tree->name);
    free(tree);
}

void free_type_trees(struct dict_entry<int32_t, unityfs_type_tree*> en) {
    free_unityfs_type_tree(en.value);
}

void free_unityfs_type_metadata(unityfs_type_metadata* meta) {
    if (!meta) return;
    linked_list_clear(meta->class_ids);
    dict_free(meta->hashes);
    dict_free(meta->type_trees, &free_type_trees);
    if (meta->generator_version) free(meta->generator_version);
    free(meta);
}

void copy_buf_str(char target[32], char buf[32]) {
    memcpy(target, buf, 32);
}

unityfs_type_metadata* create_metatree_from_asset(unityfs_archive* arc, unityfs_asset* asset) {
    if (!arc || !asset) return nullptr;
    unityfs_type_metadata* tree = (unityfs_type_metadata*)malloc(sizeof(unityfs_type_metadata));
    file_reader_file* f = nullptr;
    unityfs_type_tree* tr = nullptr;
    if (!tree) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(tree, 0, sizeof(unityfs_type_metadata));
    tree->format = asset->format;
    f = create_file_reader2((void*)arc, &unityfs_archive_block_storage_read2, &unityfs_archive_block_storage_seek2, &unityfs_archive_block_storage_tell2, asset->endian);
    if (!f) {
        printf("Out of memory.\n");
        goto end;
    }
    if (file_reader_read_str(f, &tree->generator_version)) {
        printf("Can not read metadata tree's generator version.\n");
        goto end;
    }
    if (file_reader_read_uint32(f, &tree->target_platform)) {
        printf("Can not read metadata tree's target platform.\n");
        goto end;
    }
    if (tree->format >= 13) {
        char tmp;
        if (file_reader_read_char(f, &tmp)) {
            printf("Can not read metadata tree's type trees.\n");
            goto end;
        }
        tree->has_type_trees = tmp ? 1 : 0;
        if (file_reader_read_int32(f, &tree->num_types)) {
            printf("Can not read metadata tree's num_types.\n");
            goto end;
        }
        int32_t class_id;
        char hash[32];
        for (int32_t i = 0; i < tree->num_types; i++) {
            if (file_reader_read_int32(f, &class_id)) {
                printf("Can not read metadata tree's class id.\n");
                goto end;
            }
            if (tree->format >= 17) {
                int16_t script_id;
                if (file_reader_read_char(f, NULL)) {
                    printf("Can not read data.\n");
                    goto end;
                }
                if (file_reader_read_int16(f, &script_id)) {
                    printf("Can not read metadata tree's script id.\n");
                    goto end;
                }
                if (class_id == 114) {
                    if (script_id >= 0) class_id = -2 - script_id;
                    else class_id = -1;
                }
            }
            if (!linked_list_append(tree->class_ids, &class_id)) {
                printf("Out of memory.\n");
                goto end;
            }
            if (class_id < 0) {
                if (unityfs_archive_block_storage_read(arc, 32, hash) < 32) {
                    printf("Can not read metadata tree's hash.\n");
                    goto end;
                }
            } else {
                if (unityfs_archive_block_storage_read(arc, 16, hash) < 16) {
                    printf("Can not read metadata tree's hash.\n");
                    goto end;
                }
                memset(hash + 16, 0, 16);
            }
            if (!dict_set<int32_t, char[32]>(tree->hashes, class_id, hash, &copy_buf_str)) {
                printf("Out of memory.\n");
                goto end;
            }
            if (tree->has_type_trees) {
                if (!(tr = create_typetree(arc, asset))) {
                    printf("Can not create type tree.\n");
                    goto end;
                }
                if (!dict_set(tree->type_trees, class_id, tr)) {
                    free_unityfs_type_tree(tr);
                    printf("Out of memory.\n");
                    goto end;
                }
            }
        }
    }
    if (f) free_file_reader(f);
    return tree;
end:
    if (f) free_file_reader(f);
    free_unityfs_type_metadata(tree);
    return nullptr;
}

unityfs_type_tree* create_typetree(unityfs_archive* arc, unityfs_asset* asset) {
    if (!arc) return nullptr;
    unityfs_type_tree* tree = nullptr;
    file_reader_file* f = nullptr;
    struct LinkedList<unityfs_type_tree*>* parents = nullptr;
    char* buf = nullptr;
    tree = (unityfs_type_tree*)malloc(sizeof(unityfs_type_tree));
    if (!tree) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(tree, 0, sizeof(unityfs_type_tree));
    tree->format = asset->format;
    f = create_file_reader2((void*)arc, &unityfs_archive_block_storage_read2, &unityfs_archive_block_storage_seek2, &unityfs_archive_block_storage_tell2, asset->endian);
    if (!f) {
        printf("Out of memory.\n");
        goto end;
    }
    if (tree->format == 10 || tree->format >= 12) {
        uint32_t num_nodes, buffer_bytes, nodes_byte = tree->format >= 19 ? 32 : 24;
        int64_t offset = 0, loffset = 0;
        if (file_reader_read_uint32(f, &num_nodes)) {
            goto end;
        }
        if (file_reader_read_uint32(f, &buffer_bytes)) {
            goto end;
        }
        offset = unityfs_archive_block_storage_tell(arc);
        if (offset == -1) {
            printf("Failed get current position from block storages.\n");
            goto end;
        }
        if (unityfs_archive_block_storage_seek(arc, (int64_t)nodes_byte * num_nodes, SEEK_CUR)) {
            printf("Failed seek in block storages.\n");
            goto end;
        }
        buf = (char*)malloc(buffer_bytes);
        if (!buf) {
            printf("Out of memory.\n");
            goto end;
        }
        if (unityfs_archive_block_storage_read(arc, buffer_bytes, buf) != buffer_bytes) {
            printf("Failed to read data from block storages.\n");
            goto end;
        }
        if ((loffset = unityfs_archive_block_storage_tell(arc)) == -1) {
            printf("Failed get current position from block storages.\n");
            goto end;
        }
        if (unityfs_archive_block_storage_seek(arc, offset, SEEK_SET)) {
            printf("Failed seek in block storages.\n");
            goto end;
        }
        if (!linked_list_append(parents, &tree)) {
            printf("Out of memory.\n");
            goto end;
        }
        for (uint32_t i = 0; i < num_nodes; i++) {
            int16_t version;
            uint8_t depth;
            char tmp;
            int32_t strofs;
            unityfs_type_tree* curr = nullptr;
            if (file_reader_read_int16(f, &version)) {
                printf("Failed to read type tree's version\n");
                goto end;
            }
            if (file_reader_read_uint8(f, &depth)) {
                printf("Failed to read type tree's depth\n");
                goto end;
            }
            if (!depth) {
                curr = tree;
            } else {
                while (linked_list_count(parents) > depth) {
                    linked_list_free_tail(parents);
                }
                curr = (unityfs_type_tree*)malloc(sizeof(unityfs_type_tree));
                if (!curr) {
                    printf("Out of memory.\n");
                    goto end;
                }
                memset(curr, 0, sizeof(unityfs_type_tree));
                curr->format = tree->format;
                if (!linked_list_append(linked_list_tail(parents)->d->children, &curr)) {
                    free_unityfs_type_tree(curr);
                    printf("Out of memory.\n");
                    goto end;
                }
                if (!linked_list_append(parents, &curr)) {
                    printf("Out of memory.\n");
                    goto end;
                }
            }
            curr->version = version;
            if (file_reader_read_char(f, &tmp)) {
                goto end;
            }
            curr->is_array = tmp ? 1 : 0;
            if (file_reader_read_int32(f, &strofs)) {
                printf("Failed to read type's string's offset.\n");
                goto end;
            }
            if (get_string(buf, buffer_bytes, strofs, &curr->type)) {
                printf("Failed to get type string.\n");
                goto end;
            }
            if (file_reader_read_int32(f, &strofs)) {
                printf("Failed to read name's string's offset.\n");
                goto end;
            }
            if (get_string(buf, buffer_bytes, strofs, &curr->name)) {
                printf("Failed to get name string.\n");
                goto end;
            }
            if (file_reader_read_int32(f, &curr->size)) {
                printf("Failed to read type's specified size.\n");
                goto end;
            }
            if (file_reader_read_uint32(f, &curr->index)) {
                printf("Failed to read type's index.\n");
                goto end;
            }
            if (file_reader_read_int32(f, &curr->flags)) {
                printf("Failed to read type's flags.\n");
                goto end;
            }
            if (nodes_byte > 24) {
                if (unityfs_archive_block_storage_seek(arc, nodes_byte - 24, SEEK_CUR)) {
                    printf("Failed seek in block storages.\n");
                    goto end;
                }
            }
        }
        if (unityfs_archive_block_storage_seek(arc, loffset, SEEK_SET)) {
            printf("Failed seek in block storages.\n");
            goto end;
        }
    } else {
        int32_t tmp;
        uint32_t num_fields;
        unityfs_type_tree* child;
        if (file_reader_read_str(f, &tree->type)) {
            printf("Failed to get type string.\n");
            goto end;
        }
        if (file_reader_read_str(f, &tree->name)) {
            printf("Failed to get name string.\n");
            goto end;
        }
        if (file_reader_read_int32(f, &tree->size)) {
            printf("Failed to read type's specified size.\n");
            goto end;
        }
        if (file_reader_read_uint32(f, &tree->index)) {
            printf("Failed to read type's index.\n");
            goto end;
        }
        if (file_reader_read_int32(f, &tmp)) {
            goto end;
        }
        tree->is_array = tmp ? 1 : 0;
        if (file_reader_read_int32(f, &tree->version)) {
            printf("Failed to read type tree's version\n");
            goto end;
        }
        if (file_reader_read_int32(f, &tree->flags)) {
            printf("Failed to read type's flags.\n");
            goto end;
        }
        if (file_reader_read_uint32(f, &num_fields)) {
            goto end;
        }
        for (uint32_t i = 0; i < num_fields; i++) {
            if (!(child = create_typetree(arc, asset))) {
                printf("Failed to get child type tree.\n");
                goto end;
            }
            if (!linked_list_append(tree->children, &child)) {
                printf("Out of memory.\n");
                free_unityfs_type_tree(child);
                goto end;
            }
        }
    }
    if (f) free_file_reader(f);
    if (buf) free(buf);
    linked_list_clear(parents);
    return tree;
end:
    if (f) free_file_reader(f);
    if (buf) free(buf);
    linked_list_clear(parents);
    free_unityfs_type_tree(tree);
    return nullptr;
}
