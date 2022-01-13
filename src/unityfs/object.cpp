#include "object.h"

#include <inttypes.h>
#include <malloc.h>
#include <string.h>
#include "block_storage.h"
#include "cstr_util.h"
#include "object/boolean.h"
#include "object/number.h"
#include "object_info.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void free_unityfs_object(unityfs_object* obj) {
    if (!obj) return;
    if (obj->type) {
        if (obj->data) {
            if (obj->is_number) free(obj->data);
        }
        free(obj->type);
    }
    free(obj);
}

unityfs_object* create_object_from_object_info(unityfs_object_info* obj) {
    if (!obj) return nullptr;
    file_reader_file* f = nullptr;
    unityfs_object* o = nullptr;
    unityfs_type_tree* tree = nullptr;
    if (obj->asset->arc) {
        if (!(f = create_file_reader2((void*)obj->asset->arc, &unityfs_archive_block_storage_read2, &unityfs_archive_block_storage_seek2, &unityfs_archive_block_storage_tell2, obj->asset->endian))) {
            printf("Out of memory.\n");
            goto end;
        }
        if (file_reader_seek(f, obj->asset->info->offset + obj->data_offset, SEEK_SET)) {
            printf("Failed seek in asset \"%s\".\n", obj->asset->info->name);
            goto end;
        }
    }
    tree = get_object_info_tree(obj);
    if (tree && f) {
        o = load_object(tree, f);
    }
end:
    if (f) free_file_reader(f);
    return o;
}

unityfs_object* load_object(unityfs_type_tree* tree, file_reader_file* f) {
    if (!tree || !f) return nullptr;
    unityfs_object* obj = nullptr;
    int64_t pos_before = 0, pos_after = 0;
    bool align = tree->flags & UNITYFS_TYPE_TREE_POST_ALIGN;
    obj = (unityfs_object*)malloc(sizeof(unityfs_object));
    if (!obj) {
        printf("Out of memory.\n");
        return nullptr;
    }
    memset(obj, 0, sizeof(unityfs_object));
    if (!tree->type) {
        printf("Can not acccess type tree's name.\n");
        goto end;
    }
    if (cstr_util_copy_str(&obj->type, tree->type)) {
        printf("Out of memory.\n");
        goto end;
    }
    pos_before = file_reader_tell(f);
    if (pos_before == -1) {
        printf("Failed to get current position in file.\n");
        goto end;
    }
    obj->is_number = is_number_type(obj->type) ? 1 : 0;
    if (obj->is_number) {
        if (!load_number(obj, f)) {
            goto end;
        }
        goto ok;
    }
    obj->is_bool = is_bool_type(obj->type) ? 1 : 0;
    if (obj->is_bool) {
        if (!load_boolean(obj, f)) {
            goto end;
        }
        goto ok;
    }
ok:
    pos_after = file_reader_tell(f);
    if (pos_after == -1) {
        printf("Failed to get current position in file.\n");
        goto end;
    }
    if (tree->size > 0 && (pos_after - pos_before) < tree->size) {
        printf("Type %s should have %" PRIi32 " bytes, but readed %" PRIi64 " bytes.\n", obj->type, tree->size, pos_after - pos_before);
        goto end;
    }
    if (align) {
        if (file_reader_align(f)) {
            printf("Failed to align reader's position.\n");
            goto end;
        }
    }
    return obj;
end:
    free_unityfs_object(obj);
    return nullptr;
}

void dump_unityfs_object(unityfs_object* obj, int indent, int indent_now) {
    if (!obj) return;
    if (unityfs_object_is_inline_value(obj)) return dump_unityfs_inline_object(obj, indent, indent_now);
    std::string ind(indent_now, ' ');
    printf("%sType: %s\n", ind.c_str(), obj->type);
    printf("%sValue: \n", ind.c_str());
    print_unityfs_object_value(obj, indent, indent_now + indent);
}

bool unityfs_object_is_inline_value(unityfs_object* obj) {
    if (!obj) return false;
    if (obj->is_number || obj->is_bool) return true;
    return false;
}

void dump_unityfs_inline_object(unityfs_object* obj, int indent, int indent_now) {
    if (!obj) return;
    std::string ind(indent_now, ' ');
    printf("%s(%s)", ind.c_str(), obj->type);
    print_unityfs_object_value(obj, indent, 0);
    printf("\n");
}

void print_unityfs_object_value(unityfs_object* obj, int indent, int indent_now) {
    if (!obj) return;
    if (obj->is_number) print_number(obj, indent, indent_now);
    if (obj->is_bool) print_boolean(obj, indent, indent_now);
}
