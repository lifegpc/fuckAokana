#ifndef _UNITYFS_OBJECT_INFO_H
#define _UNITYFS_OBJECT_INFO_H
#include "unityfs_private.h"
#if __cplusplus
extern "C" {
unityfs_object_info* create_object_info_from_asset(unityfs_archive* arc, unityfs_asset* asset);
unityfs_type_tree* get_object_info_tree(unityfs_object_info* obj);
#endif
#if __cplusplus
}
#endif
#endif
