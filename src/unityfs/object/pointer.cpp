#include "pointer.h"

#include <inttypes.h>
#include <malloc.h>
#include <string.h>
#include "../assetref.h"
#include "number.h"
#include "../object.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

bool is_pointer_type(std::string name) {
    auto l = name.length();
    if (l < 6) return false;
    if (name.find("PPtr<") != 0) return false;
    if (name.rfind(">") != l - 1) return false;
    return true;
}

bool check_pointer_tree(unityfs_type_tree* tree) {
    if (!tree) return false;
    if (!tree->type || !is_pointer_type(tree->type)) return false;
    if (linked_list_count(tree->children) != 2) return false;
    if (!is_number_type(tree->children->d->type) || !is_number_type(tree->children->next->d->type)) return false;
    return true;
}

bool load_pointer(unityfs_object* obj, unityfs_type_tree* tree, file_reader_file* f) {
    if (!obj || !tree || !f) return false;
    if (!check_pointer_tree(tree)) {
        printf("Type trees looks like an invalid pointer.\nThe dump of type tree: \n");
        dump_unityfs_type_tree(tree, 2, 2);
        return false;
    }
    unityfs_pointer_object* p = nullptr;
    unityfs_object* o = nullptr;
    p = (unityfs_pointer_object*)malloc(sizeof(unityfs_pointer_object));
    if (!p) {
        printf("Out of memory.\n");
        return false;
    }
    memset(p, 0, sizeof(unityfs_pointer_object));
    o = load_object(tree->children->d, f, obj->asset);
    if (!o) {
        printf("Failed to load pointer's file_id object.\n");
        goto end;
    }
    if (unityfs_object_read_uint64(o, &p->file_id)) {
        printf("Failed to get id from object.\nThe dump of object: \n");
        dump_unityfs_object(o, 2, 2);
        free_unityfs_object(o);
        goto end;
    }
    free_unityfs_object(o);
    o = load_object(tree->children->next->d, f, obj->asset);
    if (!o) {
        printf("Failed to load pointer's path_id object.\n");
        goto end;
    }
    if (unityfs_object_read_int64(o, &p->path_id)) {
        printf("Failed to get id from object.\nThe dump of object: \n");
        dump_unityfs_object(o, 2, 2);
        free_unityfs_object(o);
        goto end;
    }
    free_unityfs_object(o);
    obj->data = (void*)p;
    return true;
end:
    if (p) free(p);
    return false;
}

void print_pointer(unityfs_object* obj, int indent, int indent_now) {
    if (!obj || !obj->is_pointer || !obj->data) return;
    auto p = (unityfs_pointer_object*)obj->data;
    std::string ind(indent_now, ' ');
    printf("%sFile ID: %" PRIu64 "\n", ind.c_str(), p->file_id);
    printf("%sPath ID: %" PRIi64 "\n", ind.c_str(), p->path_id);
}

unityfs_object* unityfs_object_pointer_resolve(unityfs_object* obj) {
    if (!obj || !obj->is_pointer || !obj->data) return nullptr;
    auto p = (unityfs_pointer_object*)obj->data;
    auto ref = linked_list_get(obj->asset->asset_refs, p->file_id);
    if (!ref) {
        printf("Failed to get asset reference.\n");
        return nullptr;
    }
    auto asset = unityfs_assetref_resolve(ref->d);
    if (!asset) {
        printf("Failed to get asset.\n");
        return nullptr;
    }
    auto o = dict_get_value(asset->objects, p->path_id);
    if (!o) {
        printf("Can not find object with path_id %" PRIi64 " in asset.\n", p->path_id);
        return nullptr;
    }
    return create_object_from_object_info(o);
}
