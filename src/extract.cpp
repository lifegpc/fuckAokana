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
#include "unityfs/unityfs.h"

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
#if _WIN32
#define _S_IWRITE 0x80
#else
#define _S_IWRITE 0
#endif
#endif

#if HAVE_PRINTF_S
#define printf printf_s
#endif

bool extract_archive(std::string input, std::string output, bool overwrite, bool verbose) {
    if (!fileop::exists(input)) {
        printf("Input file \"%s\" is not exists.\n", input.c_str());
        return false;
    }
#if _WIN32
    int dir_mode = 0;
    int file_mode = _S_IWRITE;
#else
    int dir_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH;
    int file_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#endif
    if (!fileop::mkdirs(output, dir_mode, true)) {
        printf("Can not create directory: \"%s\".\n", output.c_str());
        return false;
    }
    int is_unity = 0;
    if ((is_unity = is_unityfs_file(input.c_str())) == -1) {
        printf("Can not check file \"%s\" is a unityfs file or not.\n", input.c_str());
    } else if (is_unity == 1) {
        return extract_unityfs(input, output, overwrite, verbose);
    }
    aokana_archive* arc;
    if (open_archive(arc, input)) {
        printf("Can not open archive \"%s\".\n", input.c_str());
        return false;
    }
    bool re = true;
    uint32_t count = get_file_count_from_archive(arc);
    aokana_file* f = nullptr;
    FILE* of = nullptr;
    if (verbose) {
        printf("There are %" PRIu32 " files from archive \"%s\".\n", count, input.c_str());
    }
    for (uint32_t i = 0; i < count; i++) {
        f = get_file_from_archive(arc, i);
        if (!f) {
            printf("Can not get file %" PRIu32 " from archive file: \"%s\".\n", i, input.c_str());
            re = false;
            goto end;
        }
        auto fn = get_aokana_file_name(f);
        if (verbose) {
            printf("Get file \"%s\" from archive \"%s\".\n", fn.c_str(), input.c_str());
        }
        auto path = fileop::join(output, fn);
        if (fileop::exists(path)) {
            if (!overwrite) {
                free_aokana_file(f);
                f = nullptr;
                if (verbose) {
                    printf("Output file \"%s\" already exists, skip extract file \"%s\".\n", path.c_str(), fn.c_str());
                }
                continue;
            } else {
                if (!fileop::remove(path, true)) {
                    re = false;
                    goto end;
                }
                if (verbose) {
                    printf("Removed output file \"%s\".\n", path.c_str());
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
        if ((err = fileop::open(path, fd, O_WRONLY | _O_BINARY | O_CREAT, _SH_DENYWR, file_mode))) {
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
        if (verbose) {
            printf("Writed file \"%s\" to \"%s\".\n", fn.c_str(), path.c_str());
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

bool extract_unityfs(unityfs_object* key, unityfs_object* value, std::string output, bool overwrite, bool verbose) {
    if (!key || !value) return false;
    std::string name, path;
    unityfs_object* pointer = nullptr, * obj = nullptr, * reso = nullptr;
    unityfs_streamed_resource* f = nullptr;
    FILE* of = nullptr;
    bool re = true;
    int fd = 0;
    int err = 0;
    char buf[1024];
    size_t c = 0;
#if _WIN32
    int dir_mode = 0;
    int file_mode = _S_IWRITE;
#else
    int dir_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH;
    int file_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#endif
    if (verbose) {
        printf("Key: \n");
        dump_unityfs_object(key, 2, 2);
        printf("Value: \n");
        dump_unityfs_object(value, 2, 2);
    }
    if (!unityfs_object_is_str(key)) {
        printf("Key is not string.\n");
        return false;
    }
    if (!unityfs_object_read_str(key, name)) {
        printf("Failed to read string from key.\n");
        return false;
    }
    if (!unityfs_object_is_dict(value)) {
        printf("Value is not a dict.\n");
        return false;
    }
    if (!(pointer = unityfs_object_dict_get(value, "asset"))) {
        printf("Can not get the asset.\n");
        return false;
    }
    if (!unityfs_object_is_pointer(pointer)) {
        printf("Asset is not a pointer type.\n");
        return false;
    }
    if (!(obj = unityfs_object_pointer_resolve(pointer))) {
        printf("Failed to resolve the pointer of asset.\n");
        return false;
    }
    if (verbose) {
        printf("The asset information: \n");
        dump_unityfs_object(obj, 2, 2);
    }
    if (!unityfs_object_is_dict(obj)) {
        printf("The asset is not a dict.\n");
        re = false;
        goto end;
    }
    if (!(reso = unityfs_object_dict_get(obj, "m_ExternalResources"))) {
        printf("Failed to get resource object.\n");
        re = false;
        goto end;
    }
    if (!unityfs_object_is_streamed_resource(reso)) {
        printf("Resource is not streamed resource.\n");
        re = false;
        goto end;
    }
    if (!(f = get_streamed_resource_from_object(reso))) {
        printf("Failed to get streamed resource from resouce object.\n");
        re = false;
        goto end;
    }
    path = fileop::join(output, name);
    if (fileop::exists(path)) {
        if (!overwrite) {
            if (verbose) {
                printf("Output file \"%s\" already exists, skip extract file \"%s\".\n", path.c_str(), name.c_str());
            }
            goto end;
        } else {
            if (!fileop::remove(path, true)) {
                re = false;
                goto end;
            }
            if (verbose) {
                printf("Removed output file \"%s\".\n", path.c_str());
            }
        }
    }
    if (!fileop::mkdir_for_file(path, dir_mode)) {
        auto dn = fileop::dirname(path);
        printf("Can not create directory \"%s\".\n", dn.c_str());
        re = false;
        goto end;
    }
    if ((err = fileop::open(path, fd, O_WRONLY | _O_BINARY | O_CREAT, _SH_DENYWR, file_mode))) {
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
    while (1) {
        c = unityfs_streamed_resource_read(f, 1024, buf);
        if (!c) break;
        if (fwrite(buf, 1, c, of) != c) {
            std::string errmsg;
            if (!err::get_errno_message(errmsg, errno)) {
                errmsg = "Unknown error";
            }
            printf("Can not write data to file \"%s\": \"%s\".\n", path.c_str(), errmsg.c_str());
            re = false;
            goto end;
        }
    }
    if (verbose) {
        printf("Writed file \"%s\" to \"%s\".\n", name.c_str(), path.c_str());
    }
end:
    if (obj) free_unityfs_object(obj);
    if (f) free_unityfs_streamed_resource(f);
    if (of) fileop::fclose(of);
    return re;
}

bool extract_unityfs(std::string input, std::string output, bool overwrite, bool verbose) {
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
    unityfs_archive* arc = nullptr;
    bool re = true;
    auto env = create_unityfs_environment();
    struct LinkedList<unityfs_asset*>* assets = nullptr;
    unityfs_object_info* obj = nullptr;
    unityfs_object* o = nullptr, * o2 = nullptr;
    if (!env) {
        printf("Out of memory.\n");
        re = false;
        goto end;
    }
    if (!(arc = open_unityfs_archive(env, input.c_str()))) {
        re = false;
        goto end;
    }
    if (verbose) {
        dump_unityfs_archive(arc, 2, 0);
    }
    if (!(assets = get_assets_from_archive(arc))) {
        printf("Out of memory.\n");
        re = false;
        goto end;
    }
    if (linked_list_count(assets) != 1) {
        printf("More than one assets are founded.\n");
        re = false;
        goto end;
    }
    if (!(obj = get_object_from_asset_by_path_id(assets->d, 1))) {
        printf("Can not get object from assets.\n");
        re = false;
        goto end;
    }
    if (verbose) {
        printf("The object info get from asset: \n");
        dump_unityfs_object_info(obj, 2, 2);
    }
    if (!(o = create_object_from_object_info(obj))) {
        printf("Failed to load object.\n");
        re = false;
        goto end;
    }
    if (verbose) {
        printf("The information of loaded object: \n");
        dump_unityfs_object(o, 2, 2);
    }
    if (!unityfs_object_is_dict(o)) {
        printf("The root object is not a custom type.\n");
        re = false;
        goto end;
    }
    if (!(o2 = unityfs_object_dict_get(o, "m_Container"))) {
        printf("Failed to get m_Container from object.\n");
        re = false;
        goto end;
    }
    if (verbose) {
        printf("The information of m_Container: \n");
        dump_unityfs_object(o2, 2, 2);
    }
    if (!unityfs_object_is_map(o2)) {
        printf("The container is not a map object.\n");
        re = false;
        goto end;
    }
    if (!unityfs_object_map_iter(o2, &extract_unityfs, output, overwrite, verbose)) {
        re = false;
        goto end;
    }
end:
    if (o) free_unityfs_object(o);
    if (arc) free_unityfs_archive(arc);
    if (env) free_unityfs_environment(env);
    linked_list_clear(assets);
    return re;
}
