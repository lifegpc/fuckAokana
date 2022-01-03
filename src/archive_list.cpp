#include "fuckaokana_config.h"
#include "archive_list.h"

#include <stdio.h>

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void print_get_file_info(aokana_file* f) {
    auto a = get_aokana_file_archive_name(f);
    auto b = get_aokana_file_name(f);
    printf("Get file \"%s\" from archive \"%s\".\n", b.c_str(), a.c_str());
}

void print_get_files_info(struct LinkedList<aokana_file*>* list) {
    if (!list) return;
    auto t = list;
    if (t->d) print_get_file_info(t->d);
    while (t->next) {
        t = t->next;
        if (t->d) print_get_file_info(t->d);
    }
}

aokana_file* get_file_from_archive_list(struct LinkedList<aokana_archive*>* list, std::string file_path, bool verbose) {
    if (!list) return nullptr;
    auto t = list;
    aokana_file* re = nullptr;
    if (t->d) {
        if ((re = get_file_from_archive(t->d, file_path))) {
            goto end;
        }
    }
    while (t->next) {
        t = t->next;
        if (t->d) {
            if ((re = get_file_from_archive(t->d, file_path))) {
                goto end;
            }
        }
    }
end:
    if (verbose && re) print_get_file_info(re);
    return re;
}

bool remove_empty_language_file(struct LinkedList<aokana_file*>*& list, bool verbose) {
    if (!list) return true;
    if (linked_list_count(list) == 1) return true;
    struct LinkedList<aokana_file*>* li = nullptr, * t = list, * t2 = list, * t2n = nullptr;
    aokana_file* now = t->d, * tmp = nullptr;
    int re;
    t2 = t->next;
    if (!t2) return true;
    tmp = t2->d;
    t2n = t2->next;
    re = aokana_file_compare_language(now, tmp);
    if (re == -1) {
        if (verbose) {
            printf("Remove empty language file \"%s\" from list.\n", get_aokana_file_full_path(tmp).c_str());
        }
        linked_list_remove(t2, &free_aokana_file);
    } else if (re == 1) {
        t2->d = now;
        now = tmp;
        if (verbose) {
            printf("Remove empty language file \"%s\" from list.\n", get_aokana_file_full_path(t2->d).c_str());
        }
        linked_list_remove(t2, &free_aokana_file);
    }
    while (t2n) {
        t2 = t2n;
        tmp = t2->d;
        t2n = t2->next;
        re = aokana_file_compare_language(now, tmp);
        if (re == -1) {
            if (verbose) {
                printf("Remove empty language file \"%s\" from list.", get_aokana_file_full_path(tmp).c_str());
            }
            linked_list_remove(t2, &free_aokana_file);
        } else if (re == 1) {
            t2->d = now;
            now = tmp;
            if (verbose) {
                printf("Remove empty language file \"%s\" from list.", get_aokana_file_full_path(t2->d).c_str());
            }
            linked_list_remove(t2, &free_aokana_file);
        }
    }
    if (!linked_list_append(li, &now)) {
        printf("Out of memory.\n");
        return false;
    }
    while (t->next) {
        t = t->next;
        now = t->d;
        t2 = t->next;
        if (!t2) {
            if (!linked_list_append(li, &now)) {
                linked_list_clear(li);
                printf("Out of memory.\n");
                return false;
            }
            break;
        }
        tmp = t2->d;
        t2n = t2->next;
        re = aokana_file_compare_language(now, tmp);
        if (re == -1) {
            if (verbose) {
                printf("Remove empty language file \"%s\" from list.\n", get_aokana_file_full_path(tmp).c_str());
            }
            linked_list_remove(t2, &free_aokana_file);
        } else if (re == 1) {
            t2->d = now;
            now = tmp;
            if (verbose) {
                printf("Remove empty language file \"%s\" from list.\n", get_aokana_file_full_path(t2->d).c_str());
            }
            linked_list_remove(t2, &free_aokana_file);
        }
        while (t2n) {
            t2 = t2n;
            tmp = t2->d;
            t2n = t2->next;
            re = aokana_file_compare_language(now, tmp);
            if (re == -1) {
                if (verbose) {
                    printf("Remove empty language file \"%s\" from list.\n", get_aokana_file_full_path(tmp).c_str());
                }
                linked_list_remove(t2, &free_aokana_file);
            } else if (re == 1) {
                t2->d = now;
                now = tmp;
                if (verbose) {
                    printf("Remove empty language file \"%s\" from list.\n", get_aokana_file_full_path(t2->d).c_str());
                }
                linked_list_remove(t2, &free_aokana_file);
            }
        }
        if (!linked_list_append(li, &now)) {
            linked_list_clear(li);
            printf("Out of memory.\n");
            return false;
        }
    }
    linked_list_clear(list);
    list = li;
    return true;
}

LinkedList<aokana_file*>* find_files_from_archive_list(struct LinkedList<aokana_archive*>* list, std::string language, std::string file_name, bool verbose, bool allow_fallback) {
    if (!list) return nullptr;
    auto t = list;
    LinkedList<aokana_file*>* li = nullptr, * li2 = nullptr;
    if (t->d) {
        li2 = find_files_from_archive(t->d, language, file_name, allow_fallback);
        if (li2) {
            if (!linked_list_append_list(li, li2, &compare_aokana_file_name)) {
                linked_list_clear(li, &free_aokana_file);
                linked_list_clear(li2, &free_aokana_file);
                return nullptr;
            }
            linked_list_clear(li2);
        }
    }
    while (t->next) {
        t = t->next;
        if (t->d) {
            li2 = find_files_from_archive(t->d, language, file_name, allow_fallback);
            if (li2) {
                if (!linked_list_append_list(li, li2, &compare_aokana_file_name)) {
                    linked_list_clear(li, &free_aokana_file);
                    linked_list_clear(li2, &free_aokana_file);
                    return nullptr;
                }
                linked_list_clear(li2);
            }
        }
    }
    if (!remove_empty_language_file(li, verbose)) {
        linked_list_clear(li, &free_aokana_file);
        return nullptr;
    }
    if (verbose && li) print_get_files_info(li);
    return li;
}
