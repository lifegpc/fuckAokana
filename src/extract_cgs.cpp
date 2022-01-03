#include "extract_cgs.h"
#include "fuckaokana_config.h"

#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/stat.h>
#include "archive_list.h"
#include "err.h"
#include "fileop.h"
#include "linked_list.h"
#include "merge_cg.h"
#include "str_util.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

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

bool remove_censor_file(LinkedList<aokana_file*>*& tli, bool verbose) {
    if (!tli) return true;
    LinkedList<aokana_file*>* t = tli, * li = nullptr;
    if (t->d) {
        if (aokana_file_is_censor(t->d)) {
            if (verbose) {
                printf("Remove censor file \"%s\" from list.\n", get_aokana_file_full_path(t->d).c_str());
            }
            free_aokana_file(t->d);
        } else {
            if (!linked_list_append(li, &t->d)) {
                printf("Out of memory.\n");
                return false;
            }
        }
    }
    while (t->next) {
        t = t->next;
        if (t->d) {
            if (aokana_file_is_censor(t->d)) {
                if (verbose) {
                    printf("Remove censor file \"%s\" from list.\n", get_aokana_file_full_path(t->d).c_str());
                }
                free_aokana_file(t->d);
            } else {
                if (!linked_list_append(li, &t->d)) {
                    printf("Out of memory.\n");
                    free_aokana_file(t->d);
                    while (t->next) {
                        t = t->next;
                        free_aokana_file(t->d);
                    }
                    linked_list_clear(tli);
                    linked_list_clear(li, &free_aokana_file);
                    return false;
                }
            }
        }
    }
    linked_list_clear(tli);
    tli = li;
    return true;
}

bool check_and_add_res(LinkedList<aokana_file*>*& tli, LinkedList<aokana_file*>*& hili, LinkedList<aokana_file*>*& loli, bool& have_hi, bool& have_lo, bool& first, bool verbose) {
    if (!tli) return false;
    auto c = linked_list_count(tli);
    if (c > 2) {
        if (!remove_censor_file(tli, verbose)) {
            return false;
        }
        c = linked_list_count(tli);
        if (c == 0) {
            printf("No resource files found after remove censor files.\n");
            return false;
        }
        if (c > 2) {
            printf("Too much resource files found.\n");
            return false;
        }
    }
    auto a = aokana_file_is_hi(tli->d);
    bool b = false;
    if (tli->next) b = aokana_file_is_hi(tli->next->d);
    if (first) {
        if (!tli->next) {
            have_hi = a;
            have_lo = !a;
            if (!linked_list_append(a ? hili : loli, &tli->d)) {
                printf("Out of memory.\n");
                return false;
            }
            linked_list_clear(tli);
            return true;
        } else {
            if (a == b) {
                printf("\"%s\" and \"%s\" both are high/low resources.\n", get_aokana_file_full_path(tli->d).c_str(), get_aokana_file_full_path(tli->next->d).c_str());
                return false;
            }
            have_hi = true;
            have_lo = true;
            if (!linked_list_append(a ? hili : loli, &tli->d)) {
                printf("Out of memory.\n");
                return false;
            }
            if (!linked_list_append(b ? hili : loli, &tli->next->d)) {
                printf("Out of memory.\n");
                linked_list_free_tail(a ? hili : loli);
                return false;
            }
            linked_list_clear(tli);
            return true;
        }
    } else {
        if (!tli->next) {
            if (have_hi != a || have_lo == a) {
                printf("Resource count or qualities changed.\n");
                return false;
            }
            if (!linked_list_append(a ? hili : loli, &tli->d)) {
                printf("Out of memory.\n");
                return false;
            }
            linked_list_clear(tli);
            return true;
        } else {
            if (!have_hi || !have_lo) {
                printf("Resource count or qualities changed.\n");
                return false;
            }
            if (a == b) {
                printf("\"%s\" and \"%s\" both are high/low resources.\n", get_aokana_file_full_path(tli->d).c_str(), get_aokana_file_full_path(tli->next->d).c_str());
                return false;
            }
            if (!linked_list_append(a ? hili : loli, &tli->d)) {
                printf("Out of memory.\n");
                return false;
            }
            if (!linked_list_append(b ? hili : loli, &tli->next->d)) {
                printf("Out of memory.\n");
                linked_list_free_tail(a ? hili : loli);
                return false;
            }
            linked_list_clear(tli);
            return true;
        }
    }
}

bool check_lang(std::string lang, LinkedList<aokana_file*>* li) {
    if (!li) return false;
    auto t = li;
    if (get_aokana_file_lang(t->d) == lang) return true;
    while (t->next) {
        t = t->next;
        if (get_aokana_file_lang(t->d) == lang) return true;
    }
    return false;
}

bool check_files(LinkedList<aokana_file*>*& tli, bool verbose) {
    if (!tli) return false;
    auto c = linked_list_count(tli);
    if (c > 2) {
        if (!remove_censor_file(tli, verbose)) {
            return false;
        }
        c = linked_list_count(tli);
        if (c == 0) {
            printf("No resource files found after remove censor files.\n");
            return false;
        }
        if (c > 2) {
            printf("Too much resource files found.\n");
            return false;
        }
    }
    if (!tli->next) return true;
    auto a = aokana_file_is_hi(tli->d), b = aokana_file_is_hi(tli->next->d);
    if (a == b) {
        printf("\"%s\" and \"%s\" both are high/low resources.\n", get_aokana_file_full_path(tli->d).c_str(), get_aokana_file_full_path(tli->next->d).c_str());
        return false;
    }
    return true;
}

bool extract_file(aokana_file* f, std::string output, bool overwrite, bool verbose, std::string output_filename = "") {
#if _WIN32
    int dir_mode = 0;
    int file_mode = _S_IWRITE;
#else
    int dir_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH;
    int file_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#endif
    if (!f) return false;
    auto lang = get_aokana_file_lang(f), filename = output_filename.empty() ? fileop::basename(get_aokana_file_name(f)) : output_filename;
    std::string qual = aokana_file_is_hi(f) ? "high" : "low";
    std::string full_path = fileop::join(fileop::join(fileop::join(output, qual), lang), filename);
    std::string ofn = get_aokana_file_full_path(f);
    FILE* of = nullptr;
    int fd = 0;
    int err = 0;
    bool re = true;
    char buf[1024];
    size_t c = 0;
    if (verbose) {
        printf("Extract \"%s\" to \"%s\".\n", ofn.c_str(), full_path.c_str());
    }
    if (fileop::exists(full_path)) {
        if (overwrite) {
            if (!fileop::remove(full_path, true)) {
                return false;
            }
            if (verbose) {
                printf("Removed output file \"%s\".\n", full_path.c_str());
            }
        } else {
            if (verbose) {
                printf("Output file \"%s\" already exists, skip extract file \"%s\".\n", full_path.c_str(), ofn.c_str());
            }
            return true;
        }
    }
    if (!fileop::mkdir_for_file(full_path, dir_mode)) {
        auto dn = fileop::dirname(full_path);
        printf("Can not create directory \"%s\".\n", dn.c_str());
        return false;
    }
    if ((err = fileop::open(full_path, fd, O_WRONLY | _O_BINARY | O_CREAT, _SH_DENYWR, file_mode))) {
        printf("Can not open file \"%s\".\n", full_path.c_str());
        return false;
    }
    if (!(of = fileop::fdopen(fd, "w"))) {
        printf("Can not open file \"%s\".\n", full_path.c_str());
        fileop::close(fd);
        return false;
    }
    while (1) {
        c = aokana_file_read(f, buf, 1024);
        if (c == (size_t)-1) {
            printf("Can not read data from file \"%s\".\n", ofn.c_str());
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
                printf("Can not write data to file \"%s\": \"%s\".\n", full_path.c_str(), errmsg.c_str());
                re = false;
                goto end;
            }
        }
    }
end:
    if (of) fileop::fclose(of);
    if (!re) {
        if (fileop::exists(full_path)) {
            if (fileop::remove(full_path)) {
                if (verbose) {
                    printf("Removed output file \"%s\".\n", full_path.c_str());
                }
            }
        }
    }
    return re;
}

bool prepare_merge_file(LinkedList<aokana_file*>* li, std::string output, std::string lang, bool high_res, std::string filename, bool overwrite, bool verbose) {
#if _WIN32
    int dir_mode = 0;
#else
    int dir_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH;
#endif
    if (!li) return false;
    auto full_path = fileop::join(fileop::join(fileop::join(output, high_res ? "high" : "low"), lang), filename);
    if (verbose) {
        printf("Merge files to \"%s\".\n", full_path.c_str());
    }
    if (fileop::exists(full_path)) {
        if (overwrite) {
            if (!fileop::remove(full_path, true)) {
                return false;
            }
            if (verbose) {
                printf("Removed output file \"%s\".\n", full_path.c_str());
            }
        } else {
            if (verbose) {
                printf("Output file \"%s\" already exists, skip merging to file \"%s\".\n", full_path.c_str(), filename.c_str());
            }
            return true;
        }
    }
    if (!fileop::mkdir_for_file(full_path, dir_mode)) {
        auto dn = fileop::dirname(full_path);
        printf("Can not create directory \"%s\".\n", dn.c_str());
        return false;
    }
    if (merge_cgs((aokana_file_list*)li, full_path.c_str(), nullptr, verbose)) {
        printf("Failed to merge files to \"%s\".\n", full_path.c_str());
        if (fileop::exists(full_path)) {
            if (fileop::remove(full_path)) {
                if (verbose) {
                    printf("Removed output file \"%s\".\n", full_path.c_str());
                }
            }
        }
        return false;
    }
    return true;
}

bool extract_cgs(std::string input, std::string output, bool overwrite, bool verbose) {
    auto system_path = fileop::join(input, "system.dat");
    bool re = true;
    struct LinkedList<aokana_archive*>* arcs = nullptr;
    struct aokana_file* arcs_file = nullptr, * vcg_list_file = nullptr, * cg_list_file = nullptr;
    uint32_t file_size = 0;
    char* buf = nullptr;
    std::list<std::string> arcsl, vcg_list, vcgs, cgs;
    std::list<std::string> langs = { "", "cn", "en", "jp", "tc" };
    LinkedList<aokana_file*>* tli = nullptr, * hili = nullptr, * loli = nullptr;
    if (fileop::exists(system_path)) {
        aokana_archive* arc = nullptr;
        if (open_archive(arc, system_path)) {
            printf("Warning: Can not open archive file \"%s\".\n", system_path.c_str());
        } else {
            if (!linked_list_append(arcs, &arc)) {
                re = false;
                free_archive(arc);
                printf("Out of memory.\n");
                goto end;
            }
            if (verbose) {
                printf("Opened and added archive \"%s\" to archive list.\n", system_path.c_str());
            }
        }
    } else {
        printf("system.dat not found.\n");
        return false;
    }
    arcs_file = get_file_from_archive_list(arcs, "def/arcs.txt", verbose);
    if (!arcs_file) {
        re = false;
        printf("Can not find file \"def/arcs.txt\" in archive list.\n");
        goto end;
    }
    file_size = get_aokana_file_size(arcs_file);
    buf = (char*)malloc(file_size);
    if (!buf) {
        re = false;
        printf("Out of memory.\n");
        goto end;
    }
    if (aokana_file_read(arcs_file, buf, file_size) != file_size) {
        re = false;
        printf("Can not read file \"%s\"\n", fileop::join(system_path, "def/arcs.txt").c_str());
        goto end;
    }
    arcsl = str_util::str_split(std::string(buf, file_size), "\r\n");
    free(buf);
    buf = nullptr;
    free_aokana_file(arcs_file);
    arcs_file = nullptr;
    for (auto i = arcsl.begin(); i != arcsl.end(); i++) {
        auto an = *i;
        if (an.empty()) continue;
        auto full_path = fileop::join(input, an);
        if (!fileop::exists(full_path)) {
            if (verbose) {
                printf("Skip archive \"%s\" beacuase file \"%s\" not exists.\n", an.c_str(), full_path.c_str());
            }
            continue;
        }
        aokana_archive* arc = nullptr;
        if (open_archive(arc, full_path)) {
            printf("Warning: Can not open archive file \"%s\".\n", full_path.c_str());
        } else {
            if (!linked_list_append_head(arcs, &arc)) {
                re = false;
                free_archive(arc);
                printf("Out of memory.\n");
                goto end;
            }
            if (verbose) {
                printf("Opened and added archive \"%s\" to archive list.\n", full_path.c_str());
            }
        }
    }
    arcsl.clear();
    vcg_list_file = get_file_from_archive_list(arcs, "def/vcglist.csv", verbose);
    if (!vcg_list_file) {
        re = false;
        printf("Can not find file \"def/vcglist.csv\" in archive list.\n");
        goto end;
    }
    file_size = get_aokana_file_size(vcg_list_file);
    if (buf) free(buf); // Make sure buf already freed.
    buf = (char*)malloc(file_size);
    if (!buf) {
        re = false;
        printf("Out of memory.\n");
        goto end;
    }
    if (aokana_file_read(vcg_list_file, buf, file_size) != file_size) {
        re = false;
        printf("Can not read file \"%s\"\n", get_aokana_file_full_path(vcg_list_file).c_str());
        goto end;
    }
    vcg_list = str_util::str_split(std::string(buf, file_size), "\r\n");
    free(buf);
    buf = nullptr;
    free_aokana_file(vcg_list_file);
    vcg_list_file = nullptr;
    for (auto i = vcg_list.begin(); i != vcg_list.end(); i++) {
        auto vcg = *i;
        if (vcg.empty()) continue;
        auto vcgl = str_util::str_split(vcg, " ");
        if (vcgl.size() < 2) continue;
        auto vcgt = vcgl.front();
        vcgs.push_back(vcgt);
        vcgl.pop_front();
        for (auto j = langs.begin(); j != langs.end(); j++) {
            auto lang = *j;
            bool have_hi = false, have_lo = false, first = true, ok = true;
            for (auto k = vcgl.begin(); k != vcgl.end(); k++) {
                auto cgn = *k;
                tli = find_files_from_archive_list(arcs, lang, cgn, verbose);
                if (!tli) {
                    printf("Warning: Can not find resource \"%s\" with language \"%s\" in archives.\n", cgn.c_str(), lang.c_str());
                    ok = false;
                    break;
                }
                if (!check_and_add_res(tli, hili, loli, have_hi, have_lo, first, verbose)) {
                    re = false;
                    goto end;
                }
                linked_list_clear(tli, &free_aokana_file);
            }
            if (!ok) {
                printf("Skip merge \"%s\" for language \"%s\" beacuse some resources not found. (%s)\n", vcgt.c_str(), lang.c_str(), vcg.c_str());
                linked_list_clear(hili, &free_aokana_file);
                linked_list_clear(loli, &free_aokana_file);
                continue;
            }
            if (lang.empty() || check_lang(lang, hili)) {
                auto co = linked_list_count(hili);
                if (co != vcgl.size()) {
                    printf("Warning: files in list (%zu) are less than needed (%zu).\n", co, vcgl.size());
                } else if (co == 1) {
                    if (!extract_file(hili->d, output, overwrite, verbose, vcgt + ".webp")) {
                        printf("Warning: Can not extract file \"%s\".\n", get_aokana_file_full_path(hili->d).c_str());
                    }
                } else {
                    prepare_merge_file(hili, output, lang, true, vcgt + ".webp", overwrite, verbose);
                }
            } else if (have_hi) {
                if (verbose) {
                    printf("Skip merge \"%s\" (high) for language \"%s\" because no language specified resource found.\n", vcgt.c_str(), lang.c_str());
                }
            }
            if (lang.empty() || check_lang(lang, loli)) {
                auto co = linked_list_count(loli);
                if (co != vcgl.size()) {
                    printf("Warning: files in list (%zu) are less than needed (%zu).\n", co, vcgl.size());
                } else if (co == 1) {
                    if (!extract_file(loli->d, output, overwrite, verbose, vcgt + ".webp")) {
                        printf("Warning: Can not extract file \"%s\".\n", get_aokana_file_full_path(loli->d).c_str());
                    }
                } else {
                    prepare_merge_file(loli, output, lang, false, vcgt + ".webp", overwrite, verbose);
                }
            }
            else if (have_lo) {
                if (verbose) {
                    printf("Skip merge \"%s\" (low) for language \"%s\" because no language specified resource found.\n", vcgt.c_str(), lang.c_str());
                }
            }
            linked_list_clear(hili, &free_aokana_file);
            linked_list_clear(loli, &free_aokana_file);
        }
    }
    cg_list_file = get_file_from_archive_list(arcs, "def/cglist.csv", verbose);
    if (!cg_list_file) {
        re = false;
        printf("Can not find file \"def/cglist.csv\" in archive list.\n");
        goto end;
    }
    file_size = get_aokana_file_size(cg_list_file);
    if (buf) free(buf); // Make sure buf already freed.
    buf = (char*)malloc(file_size);
    if (!buf) {
        re = false;
        printf("Out of memory.\n");
        goto end;
    }
    if (aokana_file_read(cg_list_file, buf, file_size) != file_size) {
        re = false;
        printf("Can not read file \"%s\"\n", get_aokana_file_full_path(cg_list_file).c_str());
        goto end;
    }
    cgs = str_util::str_split(std::string(buf, file_size), "\r\n");
    free(buf);
    buf = nullptr;
    free_aokana_file(cg_list_file);
    cg_list_file = nullptr;
    for (auto i = cgs.begin(); i != cgs.end(); i++) {
        auto cg = *i;
        if (cg.empty()) continue;
        auto cgl = str_util::str_split(cg, " ");
        if (cgl.size() < 3) continue;
        cgl.pop_front();
        cgl.pop_front();
        for (auto j = cgl.begin(); j != cgl.end(); j++) {
            auto cgn = *j;
            bool is_vcg = false;
            for (auto k = vcgs.begin(); k != vcgs.end(); k++) {
                if (*k == cgn) {
                    is_vcg = true;
                    break;
                }
            }
            if (is_vcg) {
                if (verbose) {
                    printf("Skip extract \"%s\" because it is a VCG.\n", cgn.c_str());
                }
                continue;
            }
            for (auto k = langs.begin(); k != langs.end(); k++) {
                auto lang = *k;
                tli = find_files_from_archive_list(arcs, lang, cgn, verbose, false);
                if (!tli) {
                    if (lang.empty()) printf("Warning: Can not find resource \"%s\" in archives.\n", cgn.c_str());
                    continue;
                }
                if (!check_files(tli, verbose)) {
                    re = false;
                    goto end;
                }
                auto t = tli;
                if (t->d) {
                    if (!extract_file(t->d, output, overwrite, verbose)) {
                        printf("Warning: Can not extract file \"%s\".\n", get_aokana_file_full_path(t->d).c_str());
                    }
                }
                while (t->next) {
                    t = t->next;
                    if (t->d) {
                        if (!extract_file(t->d, output, overwrite, verbose)) {
                            printf("Warning: Can not extract file \"%s\".\n", get_aokana_file_full_path(t->d).c_str());
                        }
                    }
                }
                linked_list_clear(tli, &free_aokana_file);
            }
        }
    }
end:
    linked_list_clear(tli, &free_aokana_file);
    linked_list_clear(hili, &free_aokana_file);
    linked_list_clear(loli, &free_aokana_file);
    if (buf) free(buf);
    if (arcs_file) free_aokana_file(arcs_file);
    if (vcg_list_file) free_aokana_file(vcg_list_file);
    if (cg_list_file) free_aokana_file(cg_list_file);
    if (arcs) linked_list_clear(arcs, &free_archive);
    return re;
}
