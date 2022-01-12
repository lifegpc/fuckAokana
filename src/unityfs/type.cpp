#include "type.h"
#include "fuckaokana_config.h"

#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "block_storage.h"
#include "cstr_util.h"
#include "dump.h"
#include "../embed_data.h"
#include "str_util.h"

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

unityfs_type_metadata* create_metatree(file_reader_file* f, uint32_t format) {
    if (!f) return nullptr;
    unityfs_type_metadata* tree = (unityfs_type_metadata*)malloc(sizeof(unityfs_type_metadata));
    unityfs_type_tree* tr = nullptr;
    if (!tree) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(tree, 0, sizeof(unityfs_type_metadata));
    tree->format = format;
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
                if (file_reader_read(f, 32, hash) < 32) {
                    printf("Can not read metadata tree's hash.\n");
                    goto end;
                }
            } else {
                if (file_reader_read(f, 16, hash) < 16) {
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
                if (!(tr = create_typetree(f, format))) {
                    printf("Can not create type tree.\n");
                    goto end;
                }
                if (!dict_set(tree->type_trees, class_id, tr)) {
                    free_unityfs_type_tree(tr);
                    printf("Out of memory.\n");
                    goto end;
                }
            }
            if (tree->format >= 21) {
                if (file_reader_seek(f, 4, SEEK_CUR)) {
                    printf("Failed seek in block storages.\n");
                    goto end;
                }
            }
        }
    } else {
        if (file_reader_read_int32(f, &tree->num_types)) {
            printf("Can not read metadata tree's num_fields .\n");
            goto end;
        }
        for (int32_t i = 0; i < tree->num_types; i++) {
            int32_t class_id;
            unityfs_type_tree* t = nullptr;
            if (file_reader_read_int32(f, &class_id)) {
                printf("Can not read metadata tree's class id.\n");
                goto end;
            }
            if (!(t = create_typetree(f, format))) {
                printf("Can not create type tree.\n");
                goto end;
            }
            if (!dict_set(tree->type_trees, class_id, t)) {
                printf("Out of memory.\n");
                free_unityfs_type_tree(t);
                goto end;
            }
        }
    }
    return tree;
end:
    free_unityfs_type_metadata(tree);
    return nullptr;
}

unityfs_type_tree* create_typetree(file_reader_file* f, uint32_t format) {
    if (!f) return nullptr;
    unityfs_type_tree* tree = nullptr;
    struct LinkedList<unityfs_type_tree*>* parents = nullptr;
    char* buf = nullptr;
    tree = (unityfs_type_tree*)malloc(sizeof(unityfs_type_tree));
    if (!tree) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(tree, 0, sizeof(unityfs_type_tree));
    tree->format = format;
    if (tree->format == 10 || tree->format >= 12) {
        uint32_t num_nodes, buffer_bytes, nodes_byte = tree->format >= 19 ? 32 : 24;
        int64_t offset = 0, loffset = 0;
        if (file_reader_read_uint32(f, &num_nodes)) {
            goto end;
        }
        if (file_reader_read_uint32(f, &buffer_bytes)) {
            goto end;
        }
        offset = file_reader_tell(f);
        if (offset == -1) {
            printf("Failed get current position from block storages.\n");
            goto end;
        }
        if (file_reader_seek(f, (int64_t)nodes_byte * num_nodes, SEEK_CUR)) {
            printf("Failed seek in block storages.\n");
            goto end;
        }
        buf = (char*)malloc(buffer_bytes);
        if (!buf) {
            printf("Out of memory.\n");
            goto end;
        }
        if (file_reader_read(f, buffer_bytes, buf) != buffer_bytes) {
            printf("Failed to read data from block storages.\n");
            goto end;
        }
        if ((loffset = file_reader_tell(f)) == -1) {
            printf("Failed get current position from block storages.\n");
            goto end;
        }
        if (file_reader_seek(f, offset, SEEK_SET)) {
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
                if (file_reader_seek(f, nodes_byte - 24, SEEK_CUR)) {
                    printf("Failed seek in block storages.\n");
                    goto end;
                }
            }
        }
        if (file_reader_seek(f, loffset, SEEK_SET)) {
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
            if (!(child = create_typetree(f, format))) {
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
    if (buf) free(buf);
    linked_list_clear(parents);
    return tree;
end:
    if (buf) free(buf);
    linked_list_clear(parents);
    free_unityfs_type_tree(tree);
    return nullptr;
}

void dump_unityfs_type_metadata_hash(int32_t key, char value[32], int indent, int indent_now) {
    std::string data = key < 0 ? std::string(value, 32) : std::string(value, 16);
    std::string ind(indent_now, ' ');
    data = str_util::str_hex(data);
    printf("%s%" PRIi32 ": 0x%s\n", ind.c_str(), key, data.c_str());
}

void dump_unityfs_type_metadata(unityfs_type_metadata* meta, int indent, int indent_now) {
    if (!meta) return;
    std::string ind(indent_now, ' ');
    std::string tmp("(Unknown)");
    printf("%sFormat: %" PRIu32 "\n", ind.c_str(), meta->format);
    dump_simple_list(meta->class_ids, "Class IDs", &print_int32, indent, indent_now, 10);
    if (meta->generator_version) tmp = meta->generator_version;
    printf("%sGenerator Version: %s\n", ind.c_str(), tmp.c_str());
    printf("%sTarget Platform: %" PRIu32 "\n", ind.c_str(), meta->target_platform);
    size_t c = dict_count(meta->hashes);
    if (c == 0) {
        printf("%sHashes: (Empty)\n", ind.c_str());
    } else {
        printf("%sHashes: \n", ind.c_str());
        dict_iter<int32_t, char[32]>(meta->hashes, &dump_unityfs_type_metadata_hash, indent, indent_now + indent);
    }
    tmp = meta->has_type_trees ? "yes" : "no";
    printf("%sHave type trees: %s\n", ind.c_str(), tmp.c_str());
    if (meta->type_trees) printf("%sType trees: \n", ind.c_str());
    dict_iter(meta->type_trees, &dump_dict, "Type tree", indent, indent_now + indent, &print_int32, &dump_unityfs_type_tree);
}

void dump_unityfs_type_tree(unityfs_type_tree* tree, int indent, int indent_now) {
    if (!tree) return;
    std::string ind(indent_now, ' ');
    printf("%sFormat: %" PRIu32 "\n", ind.c_str(), tree->format);
    printf("%sVersion: %" PRIi32 "\n", ind.c_str(), tree->version);
    std::string tmp("(Unknown)");
    if (tree->type) tmp = tree->type;
    printf("%sType: %s\n", ind.c_str(), tmp.c_str());
    tmp = tree->name ? tree->name : "(Unknown)";
    printf("%sName: %s\n", ind.c_str(), tmp.c_str());
    printf("%sSize: %" PRIi32 "\n", ind.c_str(), tree->size);
    printf("%sIndex: %" PRIu32 "\n", ind.c_str(), tree->index);
    printf("%sFlags: %" PRIi32 "\n", ind.c_str(), tree->flags);
    tmp = tree->is_array ? "yes" : "no";
    printf("%sIs array: %s\n", ind.c_str(), tmp.c_str());
    if (tree->children) printf("%sChildren: \n", ind.c_str());
    linked_list_iter(tree->children, &dump_list, (const char*)nullptr, indent, indent_now + indent, &dump_unityfs_type_tree);
}
