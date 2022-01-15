#include "string.h"

#include <malloc.h>

#if HAVE_PRINTF_S
#define printf printf_s
#endif

bool is_string_type(std::string name) {
    return name == "string";
}

bool load_string(unityfs_object* obj, file_reader_file* f, unityfs_type_tree* tree, bool& align) {
    if (!obj || !f || !tree) return false;
    uint32_t size = 0;
    if (tree->size == -1) {
        if (file_reader_read_uint32(f, &size)) {
            return false;
        }
    } else {
        size = tree->size;
    }
    char* tmp = (char*)malloc((size_t)size + 1);
    if (!tmp) {
        return false;
    }
    if (size == 0) {
        tmp[size] = 0;
        obj->data = (void*)tmp;
        align = tree->children->d->flags & UNITYFS_TYPE_TREE_POST_ALIGN;
        return true;
    }
    size_t c = 0;
    if ((c = file_reader_read(f, size, tmp)) < size) {
        free(tmp);
        return false;
    }
    if (tmp[size - 1] == 0) {
        char* ntmp = (char*)realloc(tmp, size);
        obj->data = (void*)(ntmp ? ntmp : tmp);
        align = tree->children->d->flags & UNITYFS_TYPE_TREE_POST_ALIGN;
        return true;
    } else {
        tmp[size] = 0;
        obj->data = (void*)tmp;
        align = tree->children->d->flags & UNITYFS_TYPE_TREE_POST_ALIGN;
        return true;
    }
}

void print_string(unityfs_object* obj, int indent, int indent_now) {
    if (!obj || !obj->data || !obj->is_str) return;
    char* tmp = (char*)obj->data;
    std::string ind(indent_now, ' ');
    printf("%s%s", ind.c_str(), tmp);
}

bool unityfs_object_read_str(unityfs_object* obj, std::string& s) {
    if (!obj || !obj->data || !obj->is_str) return false;
    s = std::string((char*)obj->data);
    return true;
}
