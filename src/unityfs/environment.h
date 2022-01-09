#ifndef _UNITYFS_ENVIRONMENT_H
#define _UNITYFS_ENVIRONMENT_H
#include "unityfs_private.h"
#if __cplusplus
extern "C" {
unityfs_type_metadata* get_default_type_metadata(unityfs_environment* env);
int add_archive_to_environment(unityfs_environment* env, unityfs_archive* arc);
int delete_archive_from_environment(unityfs_environment* env, unityfs_archive* arc);
#endif
#if __cplusplus
}
#endif
#endif
