#ifndef _FUCKAOKANA_EMBED_DATA_H
#define _FUCKAOKANA_EMBED_DATA_H
#if __cplusplus
extern "C" {
#endif
const unsigned char* get_strings_data();
const unsigned char* get_strings_end();
unsigned int get_strings_size();
const unsigned char* get_structs_data();
const unsigned char* get_structs_end();
unsigned int get_structs_size();
#if __cplusplus
}
#endif
#endif
