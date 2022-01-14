#include "vector.h"

#include <inttypes.h>
#include <malloc.h>
#include <string.h>
#include "number.h"
#include "../object.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void free_unityfs_vector_object(unityfs_vector_object* obj) {
    if (!obj) return;
    if (obj->obj) {
        uint64_t i = 0;
        for (; i < obj->size; i++) {
            auto n = obj->obj[i];
            if (n) free_unityfs_object(n);
        }
        free(obj->obj);
    }
    free(obj);
}

void free_unityfs_object_vector(void* data) {
    free_unityfs_vector_object((unityfs_vector_object*)data);
}

bool is_vector_type(std::string name) {
    return name == "vector";
}

bool check_vector_tree(unityfs_type_tree* tree) {
    if (!tree) return false;
    if (!tree->type || strcmp(tree->type, "vector")) return false;
    if (linked_list_count(tree->children) != 1) return false;
    auto ctr = tree->children->d;
    if (!ctr->type || strcmp(ctr->type, "Array")) return false;
    if (linked_list_count(ctr->children) != 2) return false;
    ctr = ctr->children->d;
    if (!ctr->type || !is_number_type(ctr->type)) return false;
    return true;
}

bool load_vector(unityfs_object* obj, unityfs_type_tree* tree, file_reader_file* f) {
    if (!obj || !tree || !f) return false;
    if (!check_vector_tree(tree)) {
        printf("Type trees looks like an invalid vector.\nThe dump of type tree: \n");
        dump_unityfs_type_tree(tree, 2, 2);
        return false;
    }
    unityfs_vector_object* v = nullptr;
    unityfs_object* num_obj = nullptr;
    v = (unityfs_vector_object*)malloc(sizeof(unityfs_vector_object));
    if (!v) {
        printf("Out of memory.\n");
        goto end;
    }
    memset(v, 0, sizeof(unityfs_vector_object));
    num_obj = load_object(tree->children->d->children->d, f, obj->asset);
    if (!num_obj) {
        printf("Failed to load vector's size object.\n");
        goto end;
    }
    if (unityfs_object_read_uint64(num_obj, &v->size)) {
        printf("Failed to get size from object.\nThe dump of object: \n");
        dump_unityfs_object(num_obj, 2, 2);
        free_unityfs_object(num_obj);
        goto end;
    }
    free_unityfs_object(num_obj);
    v->obj = (unityfs_object**)malloc(sizeof(unityfs_object*) * v->size);
    if (!v->obj) {
        printf("Out of memory.\n");
        goto end;
    }
    memset(v->obj, 0, sizeof(unityfs_object*) * v->size);
    for (uint64_t i = 0; i < v->size; i++) {
        unityfs_object* nobj = nullptr;
        nobj = load_object(tree->children->d->children->next->d, f, obj->asset);
        if (!nobj) {
            printf("Failed to load vector's data at index %" PRIu64 "\n", i);
            goto end;
        }
        v->obj[i] = nobj;
    }
    obj->data = (void*)v;
    return true;
end:
    if (obj) free_unityfs_vector_object(v);
    return false;
}

void print_vector(unityfs_object* obj, int indent, int indent_now) {
    if (!obj || !obj->is_vect || !obj->data) return;
    auto d = (unityfs_vector_object*)obj->data;
    std::string ind(indent_now, ' ');
    if (d->size > 0 && d->obj) {
        std::string ind2((size_t)indent_now + indent, ' ');
        for (uint64_t i = 0; i < d->size; i++) {
            auto o = d->obj[i];
            if (!o) {
                printf("%sIndex %" PRIu64 ": (null)\n", ind.c_str(), i);
            } else {
                if (unityfs_object_is_inline_value(o)) {
                    printf("%sIndex %" PRIu64 ": ", ind.c_str(), i);
                    dump_unityfs_object(o, indent, 0);
                } else {
                    printf("%sIndex %" PRIu64 ": \n", ind.c_str(), i);
                    dump_unityfs_object(o, indent, indent_now + indent);
                }
            }
        }
    }
}

size_t get_vector_count(unityfs_object* obj) {
    if (!obj || !obj->is_vect || !obj->data) return 0;
    auto d = (unityfs_vector_object*)obj->data;
    return d->size;
}
