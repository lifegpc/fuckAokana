#include "number.h"

#include <array>
#include <malloc.h>
#include <string.h>
#include "../dump.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

bool is_number_type(std::string name) {
    const std::array<std::string, 12> arr = { "SInt8", "UInt8", "SInt16", "UInt16", "SInt64", "UInt64", "SInt32", "int", "UInt32", "unsigned int", "float", "double" };
    for (auto i = arr.begin(); i != arr.end(); i++) {
        if (name == *i) return true;
    }
    return false;
}

bool load_number(unityfs_object* obj, file_reader_file* f) {
    if (!obj || !f) return false;
    std::string type(obj->type);
    int size = 0;
    if (type == "SInt8" || type == "UInt8") {
        size = 1;
    } else if (type == "SInt16" || type == "UInt16") {
        size = 2;
    } else if (type == "SInt64" || type == "UInt64" || type == "double") {
        size = 8;
    } else if (type == "SInt32" || type == "int" || type == "UInt32" || type == "unsigned int" || type == "float") {
        size = 4;
    } else {
        printf("Unknown type: %s.\n", type.c_str());
        return false;
    }
    obj->data = malloc(size);
    if (!obj->data) {
        printf("Out of memory.\n");
        return false;
    }
    memset(obj->data, 0, size);
    if (type == "SInt8" || type == "UInt8") {
        if (file_reader_read_char(f, (char*)obj->data)) {
            return false;
        }
    } else if (type == "SInt16" || type == "UInt16") {
        if (file_reader_read_int16(f, (int16_t*)obj->data)) {
            return false;
        }
    } else if (type == "SInt64" || type == "UInt64") {
        if (file_reader_read_int64(f, (int64_t*)obj->data)) {
            return false;
        }
    } else if (type == "SInt32" || type == "int" || type == "UInt32" || type == "unsigned int") {
        if (file_reader_read_int32(f, (int32_t*)obj->data)) {
            return false;
        }
    } else if (type == "float") {
        if (file_reader_align(f)) {
            return false;
        }
        if (file_reader_read_float(f, (float*)obj->data)) {
            return false;
        }
    } else if (type == "double") {
        if (file_reader_align(f)) {
            return false;
        }
        if (file_reader_read_double(f, (double*)obj->data)) {
            return false;
        }
    }
    return true;
}

void print_number(unityfs_object* obj, int indent, int indent_now) {
    if (!obj || !obj->type || !obj->data) return;
    std::string type(obj->type);
    if (type == "SInt8") print_int8(*(int8_t*)(obj->data), indent, indent_now);
    else if (type == "UInt8") print_uint8(*(uint8_t*)(obj->data), indent, indent_now);
    else if (type == "SInt16") print_int16(*(int16_t*)(obj->data), indent, indent_now);
    else if (type == "UInt16") print_uint16(*(uint16_t*)(obj->data), indent, indent_now);
    else if (type == "SInt64") print_int64(*(int64_t*)(obj->data), indent, indent_now);
    else if (type == "UInt64") print_uint64(*(uint64_t*)(obj->data), indent, indent_now);
    else if (type == "SInt32" || type == "int") print_int32(*(int32_t*)(obj->data), indent, indent_now);
    else if (type == "UInt32" || type == "unsigned int") print_uint32(*(uint32_t*)(obj->data), indent, indent_now);
    else if (type == "float") print_float(*(float*)(obj->data), indent, indent_now);
    else if (type == "double") print_double(*(double*)(obj->data), indent, indent_now);
}
