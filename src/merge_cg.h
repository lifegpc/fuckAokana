#ifndef _FUCKAOKANA_MERGE_CGS_H
#define _FUCKAOKANA_MERGE_CGS_H
#include "archive.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/dict.h"
int merge_cgs(aokana_file_list* input, const char* output, AVDictionary* options, int verbose);
#ifdef __cplusplus
}
#endif
#endif
