#include "dict.h"

#include "../utils/dict.h"
#include "../object.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void free_unityfs_object_dict(struct dict_entry<char*, unityfs_object*> k) {
    if (k.value) free_unityfs_object(k.value);
}

void free_unityfs_object_dict(void* data) {
    struct Dict<char*, unityfs_object*>* d = (struct Dict<char*, unityfs_object*>*)data;
    dict_free(d, &free_unityfs_object_dict);
}

bool load_dict(unityfs_type_tree* tree, struct Dict<char*, unityfs_object*>*& d, file_reader_file* f, unityfs_asset* asset) {
    if (!tree || !f || !tree->name || !asset) return false;
    unityfs_object* obj = load_object(tree, f, asset);
    if (!obj) {
        return false;
    }
    if (!dict_set(d, tree->name, obj, &free_unityfs_object)) {
        printf("Out of memory.\n");
        free_unityfs_object(obj);
        return false;
    }
    return true;
}

bool load_dict(unityfs_object* obj, unityfs_type_tree* tree, file_reader_file* f) {
    if (!obj || !tree || !f) return false;
    struct Dict<char*, unityfs_object*>* d = nullptr;
    bool re = linked_list_iter<unityfs_type_tree*, bool, struct Dict<char*, unityfs_object*>*&, file_reader_file*, unityfs_asset*>(tree->children, &load_dict, false, d, f, obj->asset);
    if (!re) {
        dict_free(d, &free_unityfs_object_dict);
        return false;
    }
    obj->data = (void*)d;
    return true;
}

void print_dict(char* key, unityfs_object* value, int indent, int indent_now) {
    if (!key || !value) return;
    std::string ind(indent_now, ' ');
    printf("%s%s: ", ind.c_str(), key);
    if (unityfs_object_is_inline_value(value)) {
        dump_unityfs_object(value, indent, 0);
    } else {
        printf("\n");
        dump_unityfs_object(value, indent, indent_now + indent);
    }
}

void print_dict(unityfs_object* obj, int indent, int indent_now) {
    if (!obj || !obj->is_dict || !obj->data) return;
    auto d = (struct Dict<char*, unityfs_object*>*)obj->data;
    dict_iter(d, &print_dict, indent, indent_now);
}

size_t get_dict_count(unityfs_object* obj) {
    if (!obj || !obj->is_dict || !obj->data) return 0;
    auto d = (struct Dict<char*, unityfs_object*>*)obj->data;
    return dict_count(d);
}

bool unityfs_dict_cmp_key(char* key, const char* key2) {
    return !strcmp(key, key2);
}

unityfs_object* unityfs_object_dict_get(unityfs_object* obj, const char* key) {
    if (!obj || !key || !obj->is_dict || !obj->data) return nullptr;
    auto d = (struct Dict<char*, unityfs_object*>*)obj->data;
    return dict_get_value(d, key, &unityfs_dict_cmp_key);
}
