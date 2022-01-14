#include "map.h"

#include <inttypes.h>
#include <string.h>
#include "number.h"
#include "../object.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void free_unityfs_object_map(struct dict_entry<unityfs_object*, unityfs_object*> d) {
    if (d.key) free_unityfs_object(d.key);
    if (d.value) free_unityfs_object(d.value);
}

void free_unityfs_object_map(void* data) {
    auto d = (struct Dict<unityfs_object*, unityfs_object*>*)data;
    dict_free(d, &free_unityfs_object_map);
}

bool is_map_type(std::string name) {
    return name == "map";
}

bool check_map_tree(unityfs_type_tree* tree) {
    if (!tree) return false;
    if (!tree->type || strcmp(tree->type, "map")) return false;
    if (linked_list_count(tree->children) != 1) return false;
    auto c = tree->children->d;
    if (!c->type || strcmp(c->type, "Array")) return false;
    if (linked_list_count(c->children) != 2) return false;
    if (!c->children->d->type || !is_number_type(c->children->d->type)) return false;
    c = c->children->next->d;
    if (!c->type || strcmp(c->type, "pair")) return false;
    if (linked_list_count(c->children) != 2) return false;
    return true;
}

bool load_map(unityfs_object* obj, unityfs_type_tree* tree, file_reader_file* f) {
    if (!obj || !tree || !f) return false;
    if (!check_map_tree(tree)) {
        printf("Type trees looks like an invalid map.\nThe dump of type tree: \n");
        dump_unityfs_type_tree(tree, 2, 2);
        return false;
    }
    struct Dict<unityfs_object*, unityfs_object*>* d = nullptr;
    unityfs_object* o = nullptr, * key = nullptr;
    uint64_t size = 0;
    unityfs_type_tree* nt = nullptr;
    o = load_object(tree->children->d->children->d, f, obj->asset);
    if (!o) {
        printf("Failed to load map's size object.\n");
        goto end;
    }
    if (unityfs_object_read_uint64(o, &size)) {
        printf("Failed to get size from object.\nThe dump of object: \n");
        dump_unityfs_object(o, 2, 2);
        free_unityfs_object(o);
        goto end;
    }
    free_unityfs_object(o);
    nt = tree->children->d->children->next->d;
    for (uint64_t i = 0; i < size; i++) {
        key = load_object(nt->children->d, f, obj->asset);
        if (!key) {
            printf("Failed to load map's key object at index %" PRIu64 ".\n", i);
            goto end;
        }
        o = load_object(nt->children->next->d, f, obj->asset);
        if (!o) {
            printf("Failed to load map's value object at index %" PRIu64 ".\n", i);
            free_unityfs_object(key);
            goto end;
        }
        if (!dict_set(d, key, o)) {
            printf("Out of memory.\n");
            free_unityfs_object(key);
            free_unityfs_object(o);
            goto end;
        }
    }
    obj->data = (void*)d;
    return true;
end:
    dict_free(d, &free_unityfs_object_map);
    return false;
}

void print_map(size_t index, unityfs_object* key, unityfs_object* value, int indent, int indent_now) {
    if (!key || !value) return;
    std::string ind(indent_now, ' ');
    std::string ind2((size_t)indent_now + indent, ' ');
    printf("%sPair %zu: \n", ind.c_str(), index);
    if (unityfs_object_is_inline_value(key)) {
        printf("%sKey: ", ind2.c_str());
        dump_unityfs_object(key, indent, 0);
        if (unityfs_object_is_inline_value(value)) {
            printf("%sValue: ", ind2.c_str());
            dump_unityfs_object(value, indent, 0);
        } else {
            printf("%sValue: \n", ind2.c_str());
            dump_unityfs_object(value, indent, indent_now + 2 * indent);
        }
    } else {
        printf("%sKey: \n", ind2.c_str());
        dump_unityfs_object(key, indent, indent_now + 2 * indent);
        printf("%sValue: \n", ind2.c_str());
        dump_unityfs_object(value, indent, indent_now + 2 * indent);
    }
}

void print_map(unityfs_object* obj, int indent, int indent_now) {
    if (!obj || !obj->is_map || !obj->data) return;
    auto d = (struct Dict<unityfs_object*, unityfs_object*>*)obj->data;
    dict_iter(d, &print_map, indent, indent_now);
}

size_t get_map_count(unityfs_object* obj) {
    if (!obj || !obj->is_map || !obj->data) return 0;
    auto d = (struct Dict<unityfs_object*, unityfs_object*>*)obj->data;
    return dict_count(d);
}
