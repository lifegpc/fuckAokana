#include "fuckaokana_config.h"
#include "extract.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include "err.h"
#include "fileop.h"

#include "archive.h"

#ifndef _O_BINARY
#if _WIN32
#define _O_BINARY 0x8000
#else
#define _O_BINARY 0
#endif
#endif

#ifndef _SH_DENYWR
#define _SH_DENYWR 0x20
#endif

#ifndef _S_IWRITE
#define _S_IWRITE 0x80
#endif

#if HAVE_PRINTF_S
#define printf printf_s
#endif

bool extract_archive(std::string input, std::string output, bool overwrite) {
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
    aokana_archive* arc;
    if (open_archive(arc, input)) {
        return false;
    }
    bool re = true;
    uint32_t count = get_file_count_from_archive(arc);
    aokana_file* f = nullptr;
    FILE* of = nullptr;
    for (uint32_t i = 0; i < count; i++) {
        f = get_file_from_archive(arc, i);
        if (!f) {
            printf("Can not get file %" PRIu32 " from archive file: \"%s\".\n", i, input.c_str());
            re = false;
            goto end;
        }
        auto fn = get_aokana_file_name(f);
        auto path = fileop::join(output, fn);
        if (fileop::exists(path)) {
            if (!overwrite) {
                free_aokana_file(f);
                f = nullptr;
                continue;
            } else {
                if (!fileop::remove(path, true)) {
                    re = false;
                    goto end;
                }
            }
        }
        if (!fileop::mkdir_for_file(path, dir_mode)) {
            auto dn = fileop::dirname(path);
            printf("Can not create directory \"%s\".\n", dn.c_str());
            re = false;
            goto end;
        }
        int fd = 0;
        int err = 0;
        if ((err = fileop::open(path, fd, O_WRONLY | _O_BINARY | O_CREAT, _SH_DENYWR, _S_IWRITE))) {
            printf("Can not open file \"%s\".\n", path.c_str());
            re = false;
            goto end;
        }
        if (!(of = fileop::fdopen(fd, "w"))) {
            printf("Can not open file \"%s\".\n", path.c_str());
            fileop::close(fd);
            re = false;
            goto end;
        }
        char buf[1024];
        size_t c = 0;
        while (1) {
            c = aokana_file_read(f, buf, 1024);
            if (c == (size_t)-1) {
                printf("Can not read data from file \"%s\" in archive \"%s\".\n", fn.c_str(), input.c_str());
                re = false;
                goto end;
            } else if (c == (size_t)-2) {
                break;
            }
            if (c) {
                if (!fwrite(buf, 1, c, of)) {
                    std::string errmsg;
                    if (!err::get_errno_message(errmsg, errno)) {
                        errmsg = "Unknown error";
                    }
                    printf("Can not write data to file \"%s\": \"%s\".\n", path.c_str(), errmsg.c_str());
                    re = false;
                    goto end;
                }
            }
        }
        free_aokana_file(f);
        f = nullptr;
        fileop::fclose(of);
        of = nullptr;
    }
end:
    if (f) free_aokana_file(f);
    if (of) fileop::fclose(of);
    free_archive(arc);
    return true;
}
