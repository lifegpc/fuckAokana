#ifndef _UNITYFS_DUMP_H
#define _UNITYFS_DUMP_H
#include <stdint.h>

void print_double(double data, int indent, int indent_now);
void print_float(float data, int indent, int indent_now);
void print_int8(int8_t data, int indent, int indent_now);
void print_uint8(uint8_t data, int indent, int indent_now);
void print_int16(int16_t data, int indent, int indent_now);
void print_uint16(uint16_t data, int indent, int indent_now);
void print_int32(int32_t data, int indent, int indent_now);
void print_uint32(uint32_t data, int indent, int indent_now);
void print_int64(int64_t data, int indent, int indent_now);
void print_uint64(uint64_t data, int indent, int indent_now);
#endif
