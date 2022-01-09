#include "embed_data.h"
#include "incbin.h"

INCBIN(Strings, "../data/strings.dat");
INCBIN_EXTERN(Strings);
INCBIN(Structs, "../data/structs.dat");
INCBIN_EXTERN(Structs);

const unsigned char* get_strings_data() {
    return gStringsData;
}

const unsigned char* get_strings_end() {
    return gStringsEnd;
}

unsigned int get_strings_size() {
    return gStringsSize;
}

const unsigned char* get_structs_data() {
    return gStructsData;
}

const unsigned char* get_structs_end() {
    return gStructsEnd;
}

unsigned int get_structs_size() {
    return gStructsSize;
}
