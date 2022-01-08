#include "embed_data.h"
#include "incbin.h"

INCBIN(Strings, "../data/strings.dat");
INCBIN_EXTERN(Strings);

const unsigned char* get_strings_data() {
    return gStringsData;
}

const unsigned char* get_strings_end() {
    return gStringsEnd;
}

unsigned int get_strings_size() {
    return gStringsSize;
}
