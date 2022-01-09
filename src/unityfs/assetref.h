#ifndef _UNITYFS_ASSETREF_H
#define _UNITYFS_ASSETREF_H
#include "unityfs_private.h"
#include "file_reader.h"
#if __cplusplus
extern "C" {
unityfs_assetref* create_assetref_from_asset(unityfs_asset* asset, file_reader_file* f);
#endif
#if __cplusplus
}
unityfs_assetref* create_assetref_from_asset(unityfs_asset* asset);
#endif
#endif