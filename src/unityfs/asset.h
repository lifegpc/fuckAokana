#ifndef _UNITYFS_ASSET_H
#define _UNITYFS_ASSET_H
#include "unityfs_private.h"
#if __cplusplus
extern "C" {
#endif
unityfs_asset* create_asset_from_node(unityfs_archive* arc, unityfs_node_info* inf);
#if __cplusplus
}
#endif
#endif
