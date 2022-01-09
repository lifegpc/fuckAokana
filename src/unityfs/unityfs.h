#ifndef _UNITYFS_UNITYFS_H
#define _UNITYFS_UNITYFS_H
#if __cplusplus
extern "C" {
#endif
typedef struct unityfs_archive unityfs_archive;
typedef struct unityfs_environment unityfs_environment;
/**
 * @brief Detect if a file is an unity fs file.
 * @param f file name
 * @return 1 if is an unity file otherwise 0, if an error occured, -1 is returned.
*/
int is_unityfs_file(const char* f);
void free_unityfs_archive(unityfs_archive* arc);
unityfs_archive* open_unityfs_archive(unityfs_environment* env, const char* f);
unityfs_environment* create_unityfs_environment();
void free_unityfs_environment(unityfs_environment* env);
#if __cplusplus
}
#endif
#endif
