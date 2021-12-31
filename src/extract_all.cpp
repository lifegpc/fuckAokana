#include "fuckaokana_config.h"
#include "extract_all.h"

#include <stdio.h>
#include "extract.h"
#include "fileop.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

bool extract_all_archive(std::string input, std::string output, bool overwrite, bool verbose) {
    std::list<std::string> li;
    if (!fileop::listdir(input, li)) {
        return false;
    }
    for (auto an = li.begin(); an != li.end(); an++) {
        auto arc_name = *an;
        if (arc_name.find(".dat") != (arc_name.length() - 4)) continue;
        auto arc_full_path = fileop::join(input, arc_name);
        if (verbose) {
            printf("Found archive %s (%s).\n", arc_name.c_str(), arc_full_path.c_str());
        }
        auto dir_name = arc_name;
        auto pos = dir_name.find_last_of('.');
        if (pos != std::string::npos) {
            dir_name = dir_name.substr(0, pos);
        }
        auto dir = fileop::join(output, dir_name);
        if (fileop::exists(dir)) {
            bool re;
            if (fileop::isdir(dir, re)) {
                if (!re) {
                    dir_name += "_extract";
                    dir = fileop::join(output, dir_name);
                }
            }
        }
        if (!extract_archive(arc_full_path, dir, overwrite, verbose)) {
            printf("Warning: Failed to extract archive \"%s\".\n", arc_full_path.c_str());
        }
    }
    return true;
}
