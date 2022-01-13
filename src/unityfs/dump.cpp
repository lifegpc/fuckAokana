#include "dump.h"
#include "fuckaokana_config.h"

#include <inttypes.h>
#include <stdio.h>
#include <string>

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void print_double(double data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%g", ind.c_str(), data);
}

void print_float(float data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%g", ind.c_str(), data);
}

void print_int8(int8_t data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%" PRIi8, ind.c_str(), data);
}

void print_uint8(uint8_t data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%" PRIu8, ind.c_str(), data);
}

void print_int16(int16_t data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%" PRIi16, ind.c_str(), data);
}

void print_uint16(uint16_t data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%" PRIu16, ind.c_str(), data);
}

void print_int32(int32_t data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%" PRIi32, ind.c_str(), data);
}

void print_uint32(uint32_t data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%" PRIu32, ind.c_str(), data);
}

void print_int64(int64_t data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%" PRIi64, ind.c_str(), data);
}

void print_uint64(uint64_t data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%" PRIu64, ind.c_str(), data);
}
