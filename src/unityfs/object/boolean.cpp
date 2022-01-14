#include "boolean.h"

#include <malloc.h>
#include <string.h>

#if HAVE_PRINTF_S
#define printf printf_s
#endif

bool is_bool_type(std::string name) {
    return name == "bool";
}

bool load_boolean(unityfs_object* obj, file_reader_file* f) {
    if (!obj || !f) return false;
    obj->data = malloc(1);
    if (!obj->data) {
        printf("Out of memory.\n");
        return false;
    }
    memset(obj->data, 0, 1);
    if (file_reader_read_char(f, (char*)obj->data)) {
        return false;
    }
    return true;
}

void print_boolean(unityfs_object* obj, int indent, int indent_now) {
    if (!obj || !obj->type || !obj->is_bool || !obj->data) return;
    std::string ind(indent_now, ' ');
    std::string n = *(char*)(obj->data) ? "true" : "false";
    printf("%s%s", ind.c_str(), n.c_str());
}
