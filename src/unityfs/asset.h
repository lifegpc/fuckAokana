#ifndef _UNITYFS_ASSET_H
#define _UNITYFS_ASSET_H
#include "unityfs_private.h"
#include "file_reader.h"
#if __cplusplus
extern "C" {
#endif
unityfs_asset* create_asset_from_node(unityfs_archive* arc, unityfs_node_info* inf);
/**
 * @brief Read ids from reader
 * @param asset Asset
 * @param f Reader
 * @param re Result
 * @return 1 if error orrcured otherwise 0
*/
int asset_read_id(unityfs_asset* asset, file_reader_file* f, int64_t* re);
#if __cplusplus
}
#endif
#endif
