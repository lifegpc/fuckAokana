#ifndef _UNITYFS_DECOMPRESS_H
#define _UNITYFS_DECOMPRESS_H
#if __cplusplus
extern "C" {
#endif
#include <stdint.h>
/**
 * @brief Decompress compressed data
 * @param src Compressed data
 * @param dest Uncompressed data. Need free momory by using free.
 * @param type Compress type
 * @param src_len The length of compressed data
 * @param dest_len The length of uncompressed data
 * @return 0 if successed otherwise 1
*/
int decompress(const char* src, char** dest, unsigned char type, uint32_t src_len, uint32_t* dest_len);
#if __cplusplus
}
#endif
#endif
