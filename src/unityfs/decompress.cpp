#include "decompress.h"
#include "unityfs_private.h"
#include "fuckaokana_config.h"

#include <inttypes.h>
#include <malloc.h>
#if HAVE_LZ4
#include "lz4.h"
#endif

int decompress(const char* src, char** dest, unsigned char type, uint32_t src_len, uint32_t* dest_len) {
    if (!src || !dest || !dest_len) return 1;
    if (type == UNITYFS_COMPRESSION_NONE) {
        printf("No compression type specified.\n");
        return 1;
#if HAVE_LZ4
    } else if (type == UNITYFS_COMPRESSION_LZ4 || type == UNITYFS_COMPRESSION_LZ4HC) {
        if (*dest_len == 0) {
            printf("Uncompressed size is needed for LZ4.\n");
            return 1;
        }
        char* tmp = (char*)malloc(*dest_len);
        if (!tmp) {
            printf("Out of memory.\n");
            return 1;
        }
        int re = LZ4_decompress_safe(src, tmp, src_len, *dest_len);
        if (re < 0) {
            free(tmp);
            printf("Can not decompress data with LZ4.\n");
            return 1;
        }
        if (*dest_len != re) {
            printf("Warning: Uncompressed size should be %" PRIu32 " but LZ4 returned %i.\n", *dest_len, re);
            *dest_len = re;
        }
        *dest = tmp;
        return 0;
#endif
    } else {
        printf("Unknown compression type: %hhu.\n", type);
        return 1;
    }
}
