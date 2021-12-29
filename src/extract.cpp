#include "fuckaokana_config.h"
#include "extract.h"

#include <stdio.h>
#include <sys/stat.h>
#include "fileop.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

bool extract_archive(std::string input, std::string output) {
    if (!fileop::exists(input)) {
        printf("Input file \"%s\" is not exists.\n", input.c_str());
        return false;
    }
#if _WIN32
    int dir_mode = 0;
#else
    int dir_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH;
#endif
    if (!fileop::mkdirs(output, dir_mode, true)) {
        printf("Can not create directory: \"%s\".\n", output.c_str());
        return false;
    }
    return true;
}
