#ifndef _FUCKAOKANA_ARCHIVE_H
#define _FUCKAOKANA_ARCHIVE_H
#include <stddef.h>
#include <stdint.h>
#if __cplusplus
#include <linked_list.h>
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
uint32_t get_aokana_file_size(aokana_file* f);
/**
 * @brief Compare two file's language
 * @param f file 1
 * @param f2 file 2
 * @return 0 if not same file. -1 if left is better. 1 if right is better.
*/
int aokana_file_compare_language(aokana_file* f, aokana_file* f2);
/**
 * @brief Struct are same with struct LinkedList<aokana_file_list*>
*/
typedef struct aokana_file_list {
    aokana_file* d;
    struct aokana_file_list* prev;
    struct aokana_file_list* next;
} aokana_file_list;
int aokana_file_readpacket(void* f, uint8_t* buf, int buf_size);
char* c_get_aokana_file_full_path(aokana_file* f);
#if __cplusplus
}
#include <string>
aokana_arc_error open_archive(aokana_archive*& archive, std::string path);
aokana_file* get_file_from_archive(aokana_archive* archive, uint32_t index);
std::string get_aokana_file_name(aokana_file* f);
aokana_file* get_file_from_archive(aokana_archive* archive, std::string file_path);
std::string get_archive_name(aokana_archive* archive);
std::string get_aokana_file_full_path(aokana_file* f);
/**
 * @brief Compare two file's name
 * @param f1 File 1
 * @param f2 File 2
 * @return true if same otherwise false
*/
bool compare_aokana_file_name(aokana_file* f1, aokana_file* f2);
std::string get_aokana_file_archive_name(aokana_file* f);
struct LinkedList<aokana_file*>* find_files_from_archive(aokana_archive* archive, std::string language, std::string file_name, bool allow_fallback);
bool aokana_file_is_hi(aokana_file* f);
std::string get_aokana_file_lang(aokana_file* f);
bool aokana_file_is_censor(aokana_file* f);
#endif
#endif
