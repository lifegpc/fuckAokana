#ifndef _UNITYFS_BLOCK_STORAGE_H
#define _UNITYFS_BLOCK_STORAGE_H
#include <stddef.h>
#include <stdint.h>
#include "unityfs.h"
#if __cplusplus
extern "C" {
#endif
/**
 * @brief Get the position of archive's block storage
 * @param arc archive
 * @return -1 if error occured otherwise position
*/
int64_t unityfs_archive_block_storage_tell(unityfs_archive* arc);
/**
 * @brief Check if a position is in current block
 * @param arc archive
 * @param pos position
 * @return 1 if in current block otherwise 0
*/
int unityfs_archive_block_storage_in_current_block(unityfs_archive* arc, int64_t pos);
/**
 * @brief Moves to a specified block.
 * @param arc archive
 * @param pos new position
 * @return 1 if error occured otherwise 0
*/
int unityfs_archive_block_storage_seek_to_block(unityfs_archive* arc, int64_t pos);
/**
 * @brief Moves to a specified postion
 * @param arc archive
 * @param pos new position
 * @return 1 if error occured otherwise 0
*/
int unityfs_archive_block_storage_seek3(unityfs_archive* arc, int64_t pos);
/**
 * @brief Moves to a specified postion
 * @param arc Archive
 * @param offset Number of bytes from origin.
 * @param origin Initial position. possiable value: SEEK_SET, SEEK_CUR, SEEK_END
 * @return 1 if error occured otherwise 0
*/
int unityfs_archive_block_storage_seek(unityfs_archive* arc, int64_t offset, int origin);
/**
 * @brief Read data from block storage
 * @param arc Archive
 * @param buf_len The size of buffer
 * @param buf Buffer
 * @return The number of bytes writed. 0 if error occured or end of file
*/
size_t unityfs_archive_block_storage_read(unityfs_archive* arc, size_t buf_len, char* buf);
size_t unityfs_archive_block_storage_read2(void* arc, size_t buf_len, char* buf);
int64_t unityfs_archive_block_storage_tell2(void* arc);
int unityfs_archive_block_storage_seek2(void* arc, int64_t offset, int origin);
#if __cplusplus
}
#endif
#endif
