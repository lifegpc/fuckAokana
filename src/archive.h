#ifndef _FUCKAOKANA_ARCHIVE_H
#define _FUCKAOKANA_ARCHIVE_H
#include <stddef.h>
#include <stdint.h>
#if __cplusplus
extern "C" {
#endif
typedef struct aokana_file aokana_file;
typedef struct aokana_archive aokana_archive;
typedef enum aokana_arc_error {
    AOKANA_ARC_OK,
    AOKANA_ARC_NULL_POINTER,
    AOKANA_ARC_NOT_FOUND,
    AOKANA_ARC_OOM,
    AOKANA_ARC_UNKNOWN_SIZE,
    AOKANA_ARC_OPEN_FAILED,
    AOKANA_ARC_TOO_SMALL,
} aokana_arc_error;

aokana_arc_error open_archive(aokana_archive** archive, const char* path);
void free_archive(aokana_archive* archive);
void free_aokana_file(aokana_file* f);
uint32_t get_file_count_from_archive(aokana_archive* archive);
size_t aokana_file_read(aokana_file* f, char* buf, size_t buf_len);
#if __cplusplus
}
#include <string>
aokana_arc_error open_archive(aokana_archive*& archive, std::string path);
aokana_file* get_file_from_archive(aokana_archive* archive, uint32_t index);
std::string get_aokana_file_name(aokana_file* f);
#endif
#endif
