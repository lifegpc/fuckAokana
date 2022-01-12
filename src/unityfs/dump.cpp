#include "dump.h"
#include "fuckaokana_config.h"

#include <inttypes.h>
#include <stdio.h>
#include <string>

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void print_int32(int32_t data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%" PRIi32, ind.c_str(), data);
}

void print_int64(int64_t data, int indent, int indent_now) {
    std::string ind(indent_now, ' ');
    printf("%s%" PRIi64, ind.c_str(), data);
}
